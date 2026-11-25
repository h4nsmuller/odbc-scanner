#include "columns.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "diagnostics.hpp"
#include "scanner_exception.hpp"
#include "widechar.hpp"

namespace odbcscanner {

static OdbcType GetTypeAttributes(const std::string &query, SQLSMALLINT cols_count, HSTMT hstmt, SQLUSMALLINT col_idx) {

	SQLLEN desc_type = -1;
	{
		SQLRETURN ret = SQLColAttributeW(hstmt, col_idx, SQL_DESC_TYPE, nullptr, 0, nullptr, &desc_type);
		if (!SQL_SUCCEEDED(ret)) {
			std::string diag = Diagnostics::Read(hstmt, SQL_HANDLE_STMT);
			throw ScannerException(
			    "'SQLColAttribute' for SQL_DESC_TYPE failed, column index: " + std::to_string(col_idx) +
			    ", columns count: " + std::to_string(cols_count) + ", query: '" + query +
			    "', return: " + std::to_string(ret) + ", diagnostics: '" + diag + "'");
		}
	}

	SQLLEN desc_concise_type = -1;
	{
		SQLRETURN ret =
		    SQLColAttributeW(hstmt, col_idx, SQL_DESC_CONCISE_TYPE, nullptr, 0, nullptr, &desc_concise_type);
		if (!SQL_SUCCEEDED(ret)) {
			std::string diag = Diagnostics::Read(hstmt, SQL_HANDLE_STMT);
			throw ScannerException(
			    "'SQLColAttribute' for SQL_DESC_CONCISE_TYPE failed, column index: " + std::to_string(col_idx) +
			    ", columns count: " + std::to_string(cols_count) + ", query: '" + query +
			    "', return: " + std::to_string(ret) + ", diagnostics: '" + diag + "'");
		}
	}

	SQLLEN is_unsigned = -1;
	{
		SQLRETURN ret = SQLColAttributeW(hstmt, col_idx, SQL_DESC_UNSIGNED, nullptr, 0, nullptr, &is_unsigned);
		if (!SQL_SUCCEEDED(ret)) {
			std::string diag = Diagnostics::Read(hstmt, SQL_HANDLE_STMT);
			throw ScannerException(
			    "'SQLColAttribute' for SQL_DESC_UNSIGNED failed, column index: " + std::to_string(col_idx) +
			    ", columns count: " + std::to_string(cols_count) + ", query: '" + query +
			    "', return: " + std::to_string(ret) + ", diagnostics: '" + diag + "'");
		}
	}

	std::vector<SQLWCHAR> buf;
	buf.resize(1024);
	SQLSMALLINT len_bytes = 0;
	{
		SQLRETURN ret = SQLColAttributeW(hstmt, col_idx, SQL_DESC_TYPE_NAME, buf.data(),
		                                 static_cast<SQLSMALLINT>(buf.size() * sizeof(SQLWCHAR)), &len_bytes, nullptr);
		if (!SQL_SUCCEEDED(ret)) {
			std::string diag = Diagnostics::Read(hstmt, SQL_HANDLE_STMT);
			throw ScannerException(
			    "'SQLColAttribute' for SQL_DESC_TYPE_NAME failed, column index: " + std::to_string(col_idx) +
			    ", columns count: " + std::to_string(cols_count) + ", query: '" + query +
			    "', return: " + std::to_string(ret) + ", diagnostics: '" + diag + "'");
		}
	}
	std::string desc_type_name = WideChar::Narrow(buf.data(), len_bytes / sizeof(SQLWCHAR));

	uint32_t decimal_precision = 0;
	uint32_t decimal_scale = 0;
	SQLLEN decimal_source_precision = 0;
	SQLLEN decimal_source_scale = 0;
	if (desc_concise_type == SQL_DECIMAL || desc_concise_type == SQL_NUMERIC) {
		{
			SQLLEN precision = 0;
			SQLRETURN ret = SQLColAttributeW(hstmt, col_idx, SQL_DESC_PRECISION, nullptr, 0, nullptr, &precision);
			if (!SQL_SUCCEEDED(ret)) {
				std::string diag = Diagnostics::Read(hstmt, SQL_HANDLE_STMT);
				throw ScannerException(
				    "'SQLColAttribute' for SQL_DESC_PRECISION failed, column index: " + std::to_string(col_idx) +
				    ", columns count: " + std::to_string(cols_count) + ", query: '" + query +
				    "', return: " + std::to_string(ret) + ", diagnostics: '" + diag + "'");
			}
			// Drivers report zero precision for unconstrained types (like NUMBER in Oracle), and the source type
			// can be wider than DECIMAL(38), in both cases the column is mapped to the widest fixed-size DECIMAL
			// and every fetched value is checked to fit it exactly. Columns::AddToResults widens this mapping
			// to the declared precision when DuckDB supports DECIMALs wider than 38 digits.
			SQLLEN max_precision = static_cast<SQLLEN>(OdbcType::MAX_FIXED_DECIMAL_WIDTH);
			decimal_precision =
			    static_cast<uint32_t>(precision >= 1 && precision <= max_precision ? precision : max_precision);
			decimal_source_precision = precision;
		}

		{
			SQLLEN scale = -1;
			SQLRETURN ret = SQLColAttributeW(hstmt, col_idx, SQL_DESC_SCALE, nullptr, 0, nullptr, &scale);
			if (!SQL_SUCCEEDED(ret)) {
				std::string diag = Diagnostics::Read(hstmt, SQL_HANDLE_STMT);
				throw ScannerException(
				    "'SQLColAttribute' for SQL_DESC_SCALE failed, column index: " + std::to_string(col_idx) +
				    ", columns count: " + std::to_string(cols_count) + ", query: '" + query +
				    "', return: " + std::to_string(ret) + ", diagnostics: '" + diag + "'");
			}
			if (scale < 0) {
				throw ScannerException(
				    "Decimal numbers with a negative scale cannot be represented as DuckDB's DECIMALs, scale: " +
				    std::to_string(scale) + ", column index: " + std::to_string(col_idx) + ", query: '" + query + "'");
			}
			SQLLEN precision = static_cast<SQLLEN>(decimal_precision);
			decimal_scale = static_cast<uint32_t>(scale <= precision ? scale : precision);
			decimal_source_scale = scale;
		}
	}

	return OdbcType(desc_type, desc_concise_type, is_unsigned == SQL_TRUE, std::move(desc_type_name), decimal_precision,
	                decimal_scale, decimal_source_precision, decimal_source_scale);
}

std::vector<ResultColumn> Columns::Collect(const QueryContext &ctx) {

	SQLSMALLINT cols_count = -1;
	{
		SQLRETURN ret = SQLNumResultCols(ctx.hstmt(), &cols_count);
		if (!SQL_SUCCEEDED(ret)) {
			std::string diag = Diagnostics::Read(ctx.hstmt(), SQL_HANDLE_STMT);
			throw ScannerException("'SQLNumResultCols' failed, query: '" + ctx.query +
			                       "', return: " + std::to_string(ret) + ", diagnostics: '" + diag + "'");
		}
	}

	std::vector<ResultColumn> vec;

	for (SQLSMALLINT col_idx = 1; col_idx <= cols_count; col_idx++) {
		std::vector<SQLWCHAR> buf;
		buf.resize(1024);
		SQLSMALLINT len_bytes = 0;
		{
			SQLRETURN ret =
			    SQLColAttributeW(ctx.hstmt(), col_idx, SQL_DESC_NAME, buf.data(),
			                     static_cast<SQLSMALLINT>(buf.size() * sizeof(SQLWCHAR)), &len_bytes, nullptr);
			if (!SQL_SUCCEEDED(ret)) {
				std::string diag = Diagnostics::Read(ctx.hstmt(), SQL_HANDLE_STMT);
				throw ScannerException(
				    "'SQLColAttribute' for SQL_DESC_NAME failed, column index: " + std::to_string(col_idx) +
				    ", columns count: " + std::to_string(cols_count) + ", query: '" + ctx.query +
				    "', return: " + std::to_string(ret) + ", diagnostics: '" + diag + "'");
			}
		}
		std::string name = WideChar::Narrow(buf.data(), len_bytes / sizeof(SQLWCHAR));
		OdbcType odbc_type = GetTypeAttributes(ctx.query, cols_count, ctx.hstmt(), col_idx);
		ResultColumn col(std::move(name), std::move(odbc_type));
		vec.emplace_back(std::move(col));
	}

	return vec;
}

void Columns::CheckSame(const QueryContext &ctx, const std::vector<ResultColumn> &expected,
                        const std::vector<ResultColumn> &actual) {
	if (expected.size() != actual.size()) {
		throw ScannerException("Resulting columns from 'SQLPrepare' and 'SQLExecute' do not match, query: '" +
		                       ctx.query + "',  expected count: " + std::to_string(expected.size()) +
		                       ", actual count: " + std::to_string(actual.size()));
	}
	for (size_t i = 0; i < expected.size(); i++) {
		auto &expected_col = expected.at(i);
		auto &actual_col = actual.at(i);

		if (expected_col.name != actual_col.name) {
			throw ScannerException("Resulting columns from 'SQLPrepare' and 'SQLExecute' do not match, query: '" +
			                       ctx.query + "', index: " + std::to_string(i) + ", expected name: '" +
			                       expected_col.name + "', actual name: '" + actual_col.name + "'");
		}

		// In some drivers the `PREPARE` and `EXECUTE` column types may be different,
		// lets rely on the driver to do the conversion instead of erroring out.
		/*
		if (!expected_col.odbc_type.Equals(actual_col.odbc_type)) {
		    throw ScannerException("Resulting columns from 'SQLPrepare' and 'SQLExecute' do not match, query: '" +
		                           ctx.query + "',  index: " + std::to_string(i) + ", expected: '" +
		                           expected_col.odbc_type.ToString() + "', actual: '" +
		                           actual_col.odbc_type.ToString() + "'");
		}
		*/
	}
}

// DECIMALs wider than 38 digits are not available in older DuckDB versions, and can be
// disabled in newer ones with the 'max_decimal_width' option
static bool WideDecimalsSupported(Factory &factory, uint32_t precision, uint32_t scale) {
	try {
		factory.ParseType("DECIMAL(" + std::to_string(precision) + ", " + std::to_string(scale) + ")");
		return true;
	} catch (std::exception &) {
		return false;
	}
}

// A column that is declared wider than DECIMAL(38) in the source is mapped to the DECIMAL of the same precision
// and scale when DuckDB can hold it, otherwise it stays mapped to DECIMAL(38).
static void WidenDecimalMapping(Factory &factory, ResultColumn &col) {
	OdbcType &odbc_type = col.odbc_type;
	if (!odbc_type.DecimalExceedsNumericStruct()) {
		return;
	}
	SQLLEN max_precision = static_cast<SQLLEN>(OdbcType::MAX_WIDE_DECIMAL_WIDTH);
	SQLLEN capped_precision = std::min(odbc_type.decimal_source_precision, max_precision);
	uint32_t precision = static_cast<uint32_t>(capped_precision);
	uint32_t scale = static_cast<uint32_t>(std::min(odbc_type.decimal_source_scale, capped_precision));
	if (!WideDecimalsSupported(factory, precision, scale)) {
		return;
	}
	if (odbc_type.decimal_source_precision > max_precision) {
		throw ScannerException("Unsupported DECIMAL precision: " + std::to_string(odbc_type.decimal_source_precision) +
		                       ", maximum supported precision: " + std::to_string(max_precision) + ", column name: '" +
		                       col.name + "', column type: " + odbc_type.ToString() +
		                       ". Cast this column in the remote query to a narrower DECIMAL type, " +
		                       "or to a character type to fetch it as VARCHAR.");
	}
	odbc_type.decimal_precision = precision;
	odbc_type.decimal_scale = scale;
}

void Columns::AddToResults(TableFunction::BindInput &input, Factory &factory, LogicalTypeId type_id,
                           ResultColumn &col) {
	if (type_id == LogicalTypeId::DECIMAL) {
		WidenDecimalMapping(factory, col);
		auto ltype = factory.ParseType("DECIMAL(" + std::to_string(col.odbc_type.decimal_precision) + ", " +
		                               std::to_string(col.odbc_type.decimal_scale) + ")");
		input.AddResultColumn(col.name, ltype);
	} else {
		auto ltype = factory.CreateType(type_id);
		input.AddResultColumn(col.name, ltype);
	}
}

} // namespace odbcscanner

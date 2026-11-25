#include "test_common.hpp"

static const std::string group_name = "[capi_decimal]";

// The Arrow Flight SQL ODBC driver describes every DECIMAL column as DECIMAL(19,0), whatever its declared
// precision and scale. A value with fractional digits or with more than 19 digits does not fit that type,
// and is rejected instead of being returned as a different number.
static bool RejectedByFlightSQL(Result &res, bool st) {
	if (!DBMSConfigured("FlightSQL")) {
		return false;
	}
	REQUIRE_FALSE(st);
	REQUIRE(res.Error().find("cannot be represented as DECIMAL(19,0) without changing it") != std::string::npos);
	return true;
}

TEST_CASE("Decimal INT16 query with a literal", group_name) {
	ScannerConn sc;
	Result res;
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("''1.234''", 4, 3) +
	                 R"(
  ')
)")
	                    .c_str(),
	                res);
	if (RejectedByFlightSQL(res, st)) {
		return;
	}
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int16_t>(0, 0) == 1234);
}

TEST_CASE("Decimal INT16 query with a negative literal", group_name) {
	ScannerConn sc;
	Result res;
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("''-1.234''", 4, 3) +
	                 R"(
  ')
)")
	                    .c_str(),
	                res);
	if (RejectedByFlightSQL(res, st)) {
		return;
	}
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int16_t>(0, 0) == -1234);
}

TEST_CASE("Decimal INT32 query with a literal", group_name) {
	ScannerConn sc;
	Result res;
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("''123456.789''", 9, 3) +
	                 R"(
  ')
)")
	                    .c_str(),
	                res);
	if (RejectedByFlightSQL(res, st)) {
		return;
	}
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int32_t>(0, 0) == 123456789);
}

TEST_CASE("Decimal INT32 query with a negative literal", group_name) {
	ScannerConn sc;
	Result res;
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("''-123456.789''", 9, 3) +
	                 R"(
  ')
)")
	                    .c_str(),
	                res);
	if (RejectedByFlightSQL(res, st)) {
		return;
	}
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int32_t>(0, 0) == -123456789);
}

TEST_CASE("Decimal INT64 query with a literal", group_name) {
	ScannerConn sc;
	Result res;
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("''123456789012345.123''", 18, 3) +
	                 R"(
  ')
)")
	                    .c_str(),
	                res);
	if (RejectedByFlightSQL(res, st)) {
		return;
	}
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int64_t>(0, 0) == 123456789012345123LL);
}

TEST_CASE("Decimal INT64 query with a negative literal", group_name) {
	ScannerConn sc;
	Result res;
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("''-123456789012345.123''", 18, 3) +
	                 R"(
  ')
)")
	                    .c_str(),
	                res);
	if (RejectedByFlightSQL(res, st)) {
		return;
	}
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int64_t>(0, 0) == -123456789012345123LL);
}

TEST_CASE("Decimal INT128 query with a literal", group_name) {
	if (DBMSConfigured("DB2") || DBMSConfigured("Firebird") || DBMSConfigured("Informix")) {
		return;
	}
	ScannerConn sc;
	Result res;
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("''12345678901234567890123456789012345.123''", 38, 3) +
	                 R"(
  ')
)")
	                    .c_str(),
	                res);
	if (RejectedByFlightSQL(res, st)) {
		return;
	}
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int128_t>(0, 0).lower == 14143994781733810467ULL);
	REQUIRE(res.DecimalValue<int128_t>(0, 0).upper == 669260594276348691ULL);
}

/* TODO: checkme
TEST_CASE("Decimal INT128 query with a negative 1 literal", group_name) {
    ScannerConn sc;
    Result res;
    bool st = Query(sc.conn, R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT CAST(''-1'' AS DECIMAL(38,3))
  ')
)", res);
    REQUIRE(QuerySuccess(res, st));
    REQUIRE(res.NextChunk());
    REQUIRE(res.DecimalValue<int128_t>(0, 0).lower == 18446744073709551615ULL);
    REQUIRE(res.DecimalValue<int128_t>(0, 0).upper == -1LL);
}
*/

TEST_CASE("Decimal INT128 query with a negative literal", group_name) {
	if (DBMSConfigured("DB2") || DBMSConfigured("Firebird") || DBMSConfigured("Informix")) {
		return;
	}
	ScannerConn sc;
	Result res;
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("''-12345678901234567890123456789012345.123''", 38, 3) +
	                 R"(
  ')
)")
	                    .c_str(),
	                res);
	if (RejectedByFlightSQL(res, st)) {
		return;
	}
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int128_t>(0, 0).lower == 4302749291975741149ULL);
	REQUIRE(res.DecimalValue<int128_t>(0, 0).upper == -669260594276348692LL);
}

TEST_CASE("Decimal INT16 query with a literal parameter", group_name) {
	if (DBMSConfigured("FlightSQL")) {
		return;
	}
	ScannerConn sc;
	Result res;
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("?", 4, 3, "AS col1") +
	                 R"(
  ',
  params=row('1.234'::DECIMAL(4,3)))
)")
	                    .c_str(),
	                res);
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int16_t>(0, 0) == 1234);
}

TEST_CASE("Decimal INT16 query with a negative literal parameter", group_name) {
	if (DBMSConfigured("FlightSQL")) {
		return;
	}
	ScannerConn sc;
	Result res;
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("?", 4, 3, "AS col1") +
	                 R"(
  ',
  params=row('-1.234'::DECIMAL(4,3)))
)")
	                    .c_str(),
	                res);
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int16_t>(0, 0) == -1234);
}

TEST_CASE("Decimal INT128 query with a negative literal parameter", group_name) {
	if (DBMSConfigured("DB2") || DBMSConfigured("FlightSQL") || DBMSConfigured("Firebird") ||
	    DBMSConfigured("Informix")) {
		return;
	}
	ScannerConn sc;
	Result res;
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("?", 38, 3, "AS col1") +
	                 R"(
  ',
  params=row('-12345678901234567890123456789012345.123'::DECIMAL(38,3)))
)")
	                    .c_str(),
	                res);
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int128_t>(0, 0).lower == 4302749291975741149ULL);
	REQUIRE(res.DecimalValue<int128_t>(0, 0).upper == -669260594276348692LL);
}

TEST_CASE("Decimal INT16 query with a negative parameter", group_name) {
	if (DBMSConfigured("FlightSQL")) {
		return;
	}
	ScannerConn sc;

	PreparedStatement ps;
	bool st_prepare = Prepare(sc.conn,
	                          (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("?", 4, 3, "AS col1") +
	                           R"(
  ', 
  params=row(?))
)")
	                              .c_str(),
	                          ps);
	REQUIRE(PreparedSuccess(ps, st_prepare));

	int128_t param_hugeint = {static_cast<uint64_t>(-1234LL), -1LL};
	auto param_val = CreateDecimal(sc, param_hugeint, 4, 3);
	bool st_bind = ps.Bind(1, param_val.get());
	REQUIRE(PreparedSuccess(ps, st_bind));

	Result res;
	bool st_exec = ps.Execute(res);

	REQUIRE(QuerySuccess(res, st_exec));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int16_t>(0, 0) == -1234);
}

TEST_CASE("Decimal INT128 query with a negative parameter", group_name) {
	if (DBMSConfigured("DB2") || DBMSConfigured("FlightSQL") || DBMSConfigured("Firebird") ||
	    DBMSConfigured("Informix")) {
		return;
	}
	ScannerConn sc;

	PreparedStatement ps;
	bool st_prepare = Prepare(sc.conn,
	                          (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("?", 38, 3, "AS col1") +
	                           R"(
  ', 
  params=row(?))
)")
	                              .c_str(),
	                          ps);
	REQUIRE(PreparedSuccess(ps, st_prepare));

	// -12345678901234567890123456789012345.123
	int128_t param_hugeint = {4302749291975741149ULL, -669260594276348692LL};
	auto param_val = CreateDecimal(sc, param_hugeint, 38, 3);
	bool st_bind = ps.Bind(1, param_val.get());
	REQUIRE(PreparedSuccess(ps, st_bind));

	Result res;
	bool st_exec = ps.Execute(res);

	REQUIRE(QuerySuccess(res, st_exec));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int128_t>(0, 0).lower == 4302749291975741149ULL);
	REQUIRE(res.DecimalValue<int128_t>(0, 0).upper == -669260594276348692LL);
}

TEST_CASE("Decimal INT128 query with max and min values", group_name) {
	if (!(DBMSConfigured("DuckDB") || DBMSConfigured("PostgreSQL"))) {
		return;
	}
	ScannerConn sc;
	Result res;
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("''99999999999999999999999999999999999999''", 38, 0, "AS col1") +
	                 ", " + CastAsDecimalSQL("''-99999999999999999999999999999999999999''", 38, 0, "AS col2") +
	                 R"(
  ')
)")
	                    .c_str(),
	                res);
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int128_t>(0, 0).lower == 687399551400673279ULL);
	REQUIRE(res.DecimalValue<int128_t>(0, 0).upper == 5421010862427522170LL);
	REQUIRE(res.DecimalValue<int128_t>(1, 0).lower == 17759344522308878337ULL);
	REQUIRE(res.DecimalValue<int128_t>(1, 0).upper == -5421010862427522171LL);
}

TEST_CASE("Decimal INT128 query with values fetched as chars", group_name) {
	if (!(DBMSConfigured("DuckDB") || DBMSConfigured("PostgreSQL"))) {
		return;
	}
	ScannerConn sc;
	Result res;
	// these values overflow the lower 64 bits when the last digit is appended
	bool st = Query(sc.conn,
	                (R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT )" + CastAsDecimalSQL("''36893488147419103220''", 20, 0, "AS col1") +
	                 ", " + CastAsDecimalSQL("''-36893488147419103220''", 20, 0, "AS col2") +
	                 R"(
  ',
  decimal_columns_as_chars=TRUE)
)")
	                    .c_str(),
	                res);
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.DecimalValue<int128_t>(0, 0).lower == 18446744073709551604ULL);
	REQUIRE(res.DecimalValue<int128_t>(0, 0).upper == 1LL);
	REQUIRE(res.DecimalValue<int128_t>(1, 0).lower == 12ULL);
	REQUIRE(res.DecimalValue<int128_t>(1, 0).upper == -2LL);
}

// DECIMALs wider than 38 digits are not available in older DuckDB versions
static bool WideDecimalsAvailable(ScannerConn &sc) {
	Result res;
	return Query(sc.conn, "SELECT '1'::DECIMAL(39, 0)", res);
}

TEST_CASE("Decimal wide query with literals", group_name) {
	if (!DBMSConfigured("PostgreSQL")) {
		return;
	}
	ScannerConn sc;
	if (!WideDecimalsAvailable(sc)) {
		return;
	}
	Result res;
	bool st = Query(sc.conn,
	                R"(
SELECT col1::VARCHAR, col2::VARCHAR, col3::VARCHAR, col4::VARCHAR, typeof(col1), typeof(col4)
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT
      CAST(''1234567890123456789012345678901234567890.0123456789'' AS NUMERIC(50, 10)) AS col1,
      CAST(''-1234567890123456789012345678901234567890.0123456789'' AS NUMERIC(50, 10)) AS col2,
      CAST(''0'' AS NUMERIC(50, 10)) AS col3,
      CAST(''-1.5'' AS NUMERIC(39, 1)) AS col4
  ')
)",
	                res);
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.Value<std::string>(0, 0) == "1234567890123456789012345678901234567890.0123456789");
	REQUIRE(res.Value<std::string>(1, 0) == "-1234567890123456789012345678901234567890.0123456789");
	REQUIRE(res.Value<std::string>(2, 0) == "0.0000000000");
	REQUIRE(res.Value<std::string>(3, 0) == "-1.5");
	REQUIRE(res.Value<std::string>(4, 0) == "DECIMAL(50,10)");
	REQUIRE(res.Value<std::string>(5, 0) == "DECIMAL(39,1)");
}

TEST_CASE("Decimal wide query with a NULL", group_name) {
	if (!DBMSConfigured("PostgreSQL")) {
		return;
	}
	ScannerConn sc;
	if (!WideDecimalsAvailable(sc)) {
		return;
	}
	Result res;
	bool st = Query(sc.conn,
	                R"(
SELECT col1 IS NULL
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT CAST(NULL AS NUMERIC(50, 10)) AS col1
  ')
)",
	                res);
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.Value<bool>(0, 0));
}

TEST_CASE("Decimal wide query with max width values", group_name) {
	if (!DBMSConfigured("PostgreSQL")) {
		return;
	}
	ScannerConn sc;
	if (!WideDecimalsAvailable(sc)) {
		return;
	}
	Result res;
	bool st = Query(sc.conn,
	                R"(
SELECT col1::VARCHAR, col2::VARCHAR, col3 = ('-0.' || repeat('9', 255))::DECIMAL(255, 255), typeof(col1), typeof(col3)
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT
      CAST(repeat(''9'', 255) AS NUMERIC(255, 0)) AS col1,
      CAST(''-'' || repeat(''9'', 255) AS NUMERIC(255, 0)) AS col2,
      CAST(''-0.'' || repeat(''9'', 255) AS NUMERIC(255, 255)) AS col3
  ')
)",
	                res);
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.Value<std::string>(0, 0) == std::string(255, '9'));
	REQUIRE(res.Value<std::string>(1, 0) == "-" + std::string(255, '9'));
	REQUIRE(res.Value<bool>(2, 0));
	REQUIRE(res.Value<std::string>(3, 0) == "DECIMAL(255,0)");
	REQUIRE(res.Value<std::string>(4, 0) == "DECIMAL(255,255)");
}

TEST_CASE("Decimal wide query with literal parameters", group_name) {
	if (!DBMSConfigured("PostgreSQL")) {
		return;
	}
	ScannerConn sc;
	if (!WideDecimalsAvailable(sc)) {
		return;
	}
	Result res;
	bool st = Query(sc.conn,
	                R"(
SELECT col1::VARCHAR, col2::VARCHAR
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT CAST(? AS NUMERIC(50, 10)) AS col1, CAST(? AS NUMERIC(255, 0)) AS col2
  ',
  params=row(
    '-1234567890123456789012345678901234567890.0123456789'::DECIMAL(50, 10),
    repeat('9', 255)::DECIMAL(255, 0)))
)",
	                res);
	REQUIRE(QuerySuccess(res, st));
	REQUIRE(res.NextChunk());
	REQUIRE(res.Value<std::string>(0, 0) == "-1234567890123456789012345678901234567890.0123456789");
	REQUIRE(res.Value<std::string>(1, 0) == std::string(255, '9'));
}

TEST_CASE("Decimal wide query with unsupported precision", group_name) {
	if (!DBMSConfigured("PostgreSQL")) {
		return;
	}
	ScannerConn sc;
	if (!WideDecimalsAvailable(sc)) {
		return;
	}
	Result res;
	bool st = Query(sc.conn,
	                R"(
FROM odbc_query(
  getvariable('conn'),
  '
    SELECT CAST(''1.5'' AS NUMERIC(256, 2)) AS col1
  ')
)",
	                res);
	REQUIRE(!st);
	REQUIRE(res.Error().find("Unsupported DECIMAL precision: 256, maximum supported precision: 255") !=
	        std::string::npos);
}

#include "catalog/TestSchema.h"

namespace taco {

using BasicTestSchema = TestSchema;

TEST_F(BasicTestSchema, TestNonnullableIntFields) {
    TDB_TEST_BEGIN

    auto sch = GetSchemaA();

    constexpr const int16_t f0 = -3;
    constexpr const uint64_t f1 = 0xe888888877777775ul;
    constexpr const uint8_t f2 = 'a'; // 97 == 0x61u
    constexpr const uint32_t f3 = 0xe7777777;

    maxaligned_char_buf buf;
    buf.reserve(24);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(Datum::From(f1));
    d.emplace_back(Datum::From(f2));
    d.emplace_back(Datum::From(f3));
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 24);
    ASSERT_EQ(buf.size(), 24);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 0);
    EXPECT_EQ(len[0], 2);
    EXPECT_EQ(sch->GetField(0, rec).GetInt16(), f0);

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 8);
    EXPECT_EQ(len[1], 8);
    EXPECT_EQ(sch->GetField(1, rec).GetUInt64(), f1);

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 16);
    EXPECT_EQ(len[2], 1);
    EXPECT_EQ(sch->GetField(2, rec).GetUInt8(), f2);

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 20);
    EXPECT_EQ(len[3], 4);
    EXPECT_EQ(sch->GetField(3, rec).GetUInt32(), f3);

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNonnullableFixedLenFields1) {
    TDB_TEST_BEGIN

    constexpr const uint64_t l1 = 4;
    constexpr const uint64_t l3 = 3;
    auto sch = GetSchemaB(l1, l3);

    constexpr const int16_t f0 = -3;
    constexpr const char *f1_input = "xyz";
    constexpr const uint32_t f2 = 0xe7777777;
    constexpr const char *f3_input = "ijk";

    Datum f1 = Char(f1_input, l1);
    // A CHAR field casted from an input string shorter than specified max
    // length is filled with trailing ' ' per SQL standard.
    constexpr absl::string_view f1_expected = "xyz ";
    // Note: while CHAR is a fixed-length type in SQL, its representation in
    // our program is a **variable-length** datum, because its length is a
    // runtime parameter. A more accurate description of notion of
    // variable-length in Datum, in fact, means the underlying SQL value is
    // passed by reference in an out-of-line byte buffer, instead of being
    // passed by value within the Datum object.
    ASSERT_EQ(f1.GetVarlenAsStringView(), f1_expected);

    Datum f3 = Char(f3_input, l3);
    constexpr absl::string_view f3_expected = "ijk";
    ASSERT_EQ(f3.GetVarlenAsStringView(), f3_expected);

    maxaligned_char_buf buf;
    buf.reserve(16);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(std::move(f1)); // don't use f1 from this point
    d.emplace_back(Datum::From(f2));
    d.emplace_back(std::move(f3)); // don't use f3 from this point
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 16);
    ASSERT_EQ(buf.size(), 16);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 0);
    EXPECT_EQ(len[0], 2);
    EXPECT_EQ(sch->GetField(0, rec).GetInt16(), f0);

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 2);
    EXPECT_EQ(len[1], l1);
    // Note: we are comparing whether the underlying representation of the CHAR
    // field matches our expectation, rather than asserting the equality of the
    // CHAR field values. The subtlety arises if the two CHAR fields do not
    // have matching lengths, e.g, CAST('xy' AS CHAR(2)) compares equal
    // CAST('xy' AS CHAR(3)). However, their representation are different, with
    // the former being "xy" (two bytes), and the latter being "xy " (three
    // bytes).
    //
    // Comparison of CHAR fields should normally be done
    // through its equality operator. Do not compare its underlying
    // representation directly outside this test.
    auto f1_res = sch->GetField(1, rec);
    EXPECT_EQ(f1_res.GetVarlenAsStringView(), f1_expected);

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 8);
    EXPECT_EQ(len[2], 4);
    EXPECT_EQ(sch->GetField(2, rec).GetUInt32(), f2);

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 12);
    EXPECT_EQ(len[3], l3);
    auto f3_res = sch->GetField(3, rec);
    EXPECT_EQ(f3_res.GetVarlenAsStringView(), f3_expected);

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNonnullableFixedLenFields2) {
    TDB_TEST_BEGIN

    constexpr const uint64_t l1 = 1;
    constexpr const uint64_t l3 = 9;
    auto sch = GetSchemaB(l1, l3);

    constexpr const int16_t f0 = -3;
    constexpr const char *f1_input = "x";
    constexpr const uint32_t f2 = 0xe7777777;
    constexpr const char *f3_input = "ijk";

    Datum f1 = Char(f1_input, l1);
    constexpr absl::string_view f1_expected = "x";
    ASSERT_EQ(f1.GetVarlenAsStringView(), f1_expected);

    Datum f3 = Char(f3_input, l3);
    constexpr absl::string_view f3_expected = "ijk      ";
    ASSERT_EQ(f3.GetVarlenAsStringView(), f3_expected);

    maxaligned_char_buf buf;
    buf.reserve(24);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(std::move(f1)); // don't use f1 from this point
    d.emplace_back(Datum::From(f2));
    d.emplace_back(std::move(f3)); // don't use f3 from this point
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 24);
    ASSERT_EQ(buf.size(), 24);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 0);
    EXPECT_EQ(len[0], 2);
    EXPECT_EQ(sch->GetField(0, rec).GetInt16(), f0);

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 2);
    EXPECT_EQ(len[1], l1);
    auto f1_res = sch->GetField(1, rec);
    EXPECT_EQ(f1_res.GetVarlenAsStringView(), f1_expected);

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 4);
    EXPECT_EQ(len[2], 4);
    EXPECT_EQ(sch->GetField(2, rec).GetUInt32(), f2);

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 8);
    EXPECT_EQ(len[3], l3);
    auto f3_res = sch->GetField(3, rec);
    EXPECT_EQ(f3_res.GetVarlenAsStringView(), f3_expected);

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNonnullableFixedLenFields3) {
    TDB_TEST_BEGIN

    constexpr const uint64_t l1 = 20;
    constexpr const uint64_t l3 = 132;
    auto sch = GetSchemaB(l1, l3);

    constexpr const int16_t f0 = -3;
    constexpr const char *f1_input = "xyzwvwopr";
    constexpr const uint32_t f2 = 0xe7777777;
    constexpr const char *f3_input = "abcdefghijk";

    Datum f1 = Char(f1_input, l1);
    std::string f1_expected_ = f1_input;
    f1_expected_.resize(l1, ' ');
    absl::string_view f1_expected = f1_expected_;
    ASSERT_EQ(f1.GetVarlenAsStringView(), f1_expected);

    Datum f3 = Char(f3_input, l3);
    std::string f3_expected_ = f3_input;
    f3_expected_.resize(l3, ' ');
    absl::string_view f3_expected = f3_expected_;
    ASSERT_EQ(f3.GetVarlenAsStringView(), f3_expected);

    maxaligned_char_buf buf;
    buf.reserve(160);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(std::move(f1)); // don't use f1 from this point
    d.emplace_back(Datum::From(f2));
    d.emplace_back(std::move(f3)); // don't use f3 from this point
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 160);
    ASSERT_EQ(buf.size(), 160);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 0);
    EXPECT_EQ(len[0], 2);
    EXPECT_EQ(sch->GetField(0, rec).GetInt16(), f0);

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 2);
    EXPECT_EQ(len[1], l1);
    auto f1_res = sch->GetField(1, rec);
    EXPECT_EQ(f1_res.GetVarlenAsStringView(), f1_expected);

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 24);
    EXPECT_EQ(len[2], 4);
    EXPECT_EQ(sch->GetField(2, rec).GetUInt32(), f2);

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 28);
    EXPECT_EQ(len[3], l3);
    auto f3_res = sch->GetField(3, rec);
    EXPECT_EQ(f3_res.GetVarlenAsStringView(), f3_expected);

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNonnullableVarlenFields1) {
    TDB_TEST_BEGIN

    auto sch = GetSchemaC();

    constexpr const int16_t f0 = 0x7777;
    constexpr absl::string_view f1_input = "xy";
    constexpr const uint8_t f2 = 'b'; // 98 == 0x62
    constexpr absl::string_view f3_input = "1234567890";

    Datum f1 = Varchar(f1_input);
    ASSERT_EQ(f1.GetVarlenAsStringView(), f1_input);

    Datum f3 = Decimal(f3_input);
    Datum f3_out = DecimalToString(f3);
    ASSERT_EQ(f3_out.GetVarlenAsStringView(), f3_input);

    maxaligned_char_buf buf;
    buf.reserve(24);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(std::move(f1)); // don't use f1 from this point
    d.emplace_back(Datum::From(f2));
    d.emplace_back(std::move(f3)); // don't use f3 from this point
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 24);
    ASSERT_EQ(buf.size(), 24);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 0);
    EXPECT_EQ(len[0], 2);
    EXPECT_EQ(sch->GetField(0, rec).GetInt16(), f0);

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 8);
    EXPECT_EQ(len[1], 2);
    auto f1_res = sch->GetField(1, rec);
    EXPECT_EQ(f1_res.GetVarlenAsStringView(), f1_input);

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 2);
    EXPECT_EQ(len[2], 1);
    EXPECT_EQ(sch->GetField(2, rec).GetUInt8(), f2);

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 12);
    EXPECT_EQ(len[3], 8);
    auto f3_res = sch->GetField(3, rec);
    auto f3_res_out = DecimalToString(f3_res);
    EXPECT_EQ(f3_res_out.GetVarlenAsStringView(),
              f3_out.GetVarlenAsStringView());

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNonnullableVarlenFields2) {
    TDB_TEST_BEGIN

    auto sch = GetSchemaC();

    constexpr const int16_t f0 = 0x7777;
    constexpr absl::string_view f1_input = "";
    constexpr const uint8_t f2 = 'b'; // 98 == 0x62
    constexpr absl::string_view f3_input = "10";

    Datum f1 = Varchar(f1_input);
    ASSERT_EQ(f1.GetVarlenAsStringView(), f1_input);

    Datum f3 = Decimal(f3_input);
    Datum f3_out = DecimalToString(f3);
    ASSERT_EQ(f3_out.GetVarlenAsStringView(), f3_input);

    maxaligned_char_buf buf;
    buf.reserve(16);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(std::move(f1)); // don't use f1 from this point
    d.emplace_back(Datum::From(f2));
    d.emplace_back(std::move(f3)); // don't use f3 from this point
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 16);
    ASSERT_EQ(buf.size(), 16);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 0);
    EXPECT_EQ(len[0], 2);
    EXPECT_EQ(sch->GetField(0, rec).GetInt16(), f0);

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 8);
    EXPECT_EQ(len[1], 0);
    auto f1_res = sch->GetField(1, rec);
    EXPECT_EQ(f1_res.GetVarlenAsStringView(), f1_input);

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 2);
    EXPECT_EQ(len[2], 1);
    EXPECT_EQ(sch->GetField(2, rec).GetUInt8(), f2);

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 8);
    EXPECT_EQ(len[3], 4);
    auto f3_res = sch->GetField(3, rec);
    auto f3_res_out = DecimalToString(f3_res);
    EXPECT_EQ(f3_res_out.GetVarlenAsStringView(),
              f3_out.GetVarlenAsStringView());

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNonnullableVarlenFields3) {
    TDB_TEST_BEGIN

    auto sch = GetSchemaD();

    constexpr const uint32_t f0 = 0xfedcba98;
    constexpr absl::string_view f1_input = "100001";
    constexpr const int16_t f2 = 0x7777;
    constexpr absl::string_view f3_input = "abe";

    Datum f1 = Decimal(f1_input);
    Datum f1_out = DecimalToString(f1);
    ASSERT_EQ(f1_out.GetVarlenAsStringView(), f1_input);

    Datum f3 = Varchar(f3_input);
    ASSERT_EQ(f3.GetVarlenAsStringView(), f3_input);

    maxaligned_char_buf buf;
    buf.reserve(24);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(std::move(f1)); // don't use f1 from this point
    d.emplace_back(Datum::From(f2));
    d.emplace_back(std::move(f3)); // don't use f3 from this point
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 24);
    ASSERT_EQ(buf.size(), 24);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 0);
    EXPECT_EQ(len[0], 4);
    EXPECT_EQ(sch->GetField(0, rec).GetUInt32(), f0);

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 12);
    EXPECT_EQ(len[1], 8);
    auto f1_res = sch->GetField(1, rec);
    auto f1_res_out = DecimalToString(f1_res);
    EXPECT_EQ(f1_res_out.GetVarlenAsStringView(),
              f1_out.GetVarlenAsStringView());

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 4);
    EXPECT_EQ(len[2], 2);
    EXPECT_EQ(sch->GetField(2, rec).GetInt16(), f2);

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 20);
    EXPECT_EQ(len[3], 3);
    auto f3_res = sch->GetField(3, rec);
    EXPECT_EQ(f3_res.GetVarlenAsStringView(), f3_input);

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNullableIntFields1) {
    TDB_TEST_BEGIN

    auto sch = GetSchemaAWithNullable();

    constexpr const int16_t f0 = -3;
    constexpr const uint64_t f1 = 0xe888888877777775ul;
    constexpr const uint8_t f2 = 'a'; // 97 == 0x61u
    constexpr const uint32_t f3 = 0xe7777777;

    maxaligned_char_buf buf;
    buf.reserve(16);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(Datum::From(f1));
    d.emplace_back(Datum::From(f2));
    d.emplace_back(Datum::From(f3));
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 16);
    ASSERT_EQ(buf.size(), 16);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 10);
    EXPECT_EQ(len[0], 2);
    EXPECT_EQ(sch->GetField(0, rec).GetInt16(), f0);

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 0);
    EXPECT_EQ(len[1], 8);
    EXPECT_EQ(sch->GetField(1, rec).GetUInt64(), f1);

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 8);
    EXPECT_EQ(len[2], 1);
    EXPECT_EQ(sch->GetField(2, rec).GetUInt8(), f2);

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 12);
    EXPECT_EQ(len[3], 4);
    EXPECT_EQ(sch->GetField(3, rec).GetUInt32(), f3);

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNullableIntFields2) {
    TDB_TEST_BEGIN

    auto sch = GetSchemaAWithNullable();

    constexpr const uint64_t f1 = 0xe888888877777775ul;
    constexpr const uint8_t f2 = 'a'; // 97 == 0x61u
    constexpr const uint32_t f3 = 0xe7777777;

    maxaligned_char_buf buf;
    buf.reserve(16);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::FromNull());
    d.emplace_back(Datum::From(f1));
    d.emplace_back(Datum::From(f2));
    d.emplace_back(Datum::From(f3));
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 16);
    ASSERT_EQ(buf.size(), 16);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_TRUE(sch->FieldIsNull(0, rec));

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 0);
    EXPECT_EQ(len[1], 8);
    EXPECT_EQ(sch->GetField(1, rec).GetUInt64(), f1);

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 8);
    EXPECT_EQ(len[2], 1);
    EXPECT_EQ(sch->GetField(2, rec).GetUInt8(), f2);

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 12);
    EXPECT_EQ(len[3], 4);
    EXPECT_EQ(sch->GetField(3, rec).GetUInt32(), f3);

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNullableIntFields3) {
    TDB_TEST_BEGIN

    auto sch = GetSchemaAWithNullable();

    constexpr const int16_t f0 = -3;
    constexpr const uint64_t f1 = 0xe888888877777775ul;
    constexpr const uint8_t f2 = 'a'; // 97 == 0x61u

    maxaligned_char_buf buf;
    buf.reserve(16);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(Datum::From(f1));
    d.emplace_back(Datum::From(f2));
    d.emplace_back(Datum::FromNull());
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 16);
    ASSERT_EQ(buf.size(), 16);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 10);
    EXPECT_EQ(len[0], 2);
    EXPECT_EQ(sch->GetField(0, rec).GetInt16(), f0);

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 0);
    EXPECT_EQ(len[1], 8);
    EXPECT_EQ(sch->GetField(1, rec).GetUInt64(), f1);

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 8);
    EXPECT_EQ(len[2], 1);
    EXPECT_EQ(sch->GetField(2, rec).GetUInt8(), f2);

    ASSERT_TRUE(sch->FieldIsNull(3, rec));

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNullableIntFields4) {
    TDB_TEST_BEGIN

    auto sch = GetSchemaAWithNullable();

    constexpr const uint64_t f1 = 0xe888888877777775ul;
    constexpr const uint8_t f2 = 'a'; // 97 == 0x61u

    maxaligned_char_buf buf;
    buf.reserve(16);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::FromNull());
    d.emplace_back(Datum::From(f1));
    d.emplace_back(Datum::From(f2));
    d.emplace_back(Datum::FromNull());
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 16);
    ASSERT_EQ(buf.size(), 16);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_TRUE(sch->FieldIsNull(0, rec));

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 0);
    EXPECT_EQ(len[1], 8);
    EXPECT_EQ(sch->GetField(1, rec).GetUInt64(), f1);

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 8);
    EXPECT_EQ(len[2], 1);
    EXPECT_EQ(sch->GetField(2, rec).GetUInt8(), f2);

    ASSERT_TRUE(sch->FieldIsNull(3, rec));

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNullableVarlenFields1) {
    TDB_TEST_BEGIN

    auto sch = GetSchemaDWithNullable();

    constexpr const uint32_t f0 = 0xfedcba98;
    constexpr absl::string_view f1_input = "100001";
    constexpr const int16_t f2 = 0x7777;
    constexpr absl::string_view f3_input = "abe";

    Datum f1 = Decimal(f1_input);
    Datum f1_out = DecimalToString(f1);
    ASSERT_EQ(f1_out.GetVarlenAsStringView(), f1_input);

    Datum f3 = Varchar(f3_input);
    ASSERT_EQ(f3.GetVarlenAsStringView(), f3_input);

    maxaligned_char_buf buf;
    buf.reserve(32);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(std::move(f1)); // don't use f1 from this point
    d.emplace_back(Datum::From(f2));
    d.emplace_back(std::move(f3)); // don't use f3 from this point
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 32);
    ASSERT_EQ(buf.size(), 32);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 0);
    EXPECT_EQ(len[0], 4);
    EXPECT_EQ(sch->GetField(0, rec).GetUInt32(), f0);

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 12);
    EXPECT_EQ(len[1], 8);
    auto f1_res = sch->GetField(1, rec);
    auto f1_res_out = DecimalToString(f1_res);
    EXPECT_EQ(f1_res_out.GetVarlenAsStringView(),
              f1_out.GetVarlenAsStringView());

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 24);
    EXPECT_EQ(len[2], 2);
    EXPECT_EQ(sch->GetField(2, rec).GetInt16(), f2);

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 20);
    EXPECT_EQ(len[3], 3);
    auto f3_res = sch->GetField(3, rec);
    EXPECT_EQ(f3_res.GetVarlenAsStringView(), f3_input);

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNullableVarlenFields2) {
    TDB_TEST_BEGIN

    auto sch = GetSchemaDWithNullable();

    constexpr const uint32_t f0 = 0xfedcba98;
    constexpr const int16_t f2 = 0x7777;
    constexpr absl::string_view f3_input = "abe";

    Datum f3 = Varchar(f3_input);
    ASSERT_EQ(f3.GetVarlenAsStringView(), f3_input);

    maxaligned_char_buf buf;
    buf.reserve(16);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(Datum::FromNull());
    d.emplace_back(Datum::From(f2));
    d.emplace_back(std::move(f3)); // don't use f3 from this point
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 16);
    ASSERT_EQ(buf.size(), 16);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 0);
    EXPECT_EQ(len[0], 4);
    EXPECT_EQ(sch->GetField(0, rec).GetUInt32(), f0);

    ASSERT_TRUE(sch->FieldIsNull(1, rec));

    ASSERT_FALSE(sch->FieldIsNull(2, rec));
    std::tie(off[2], len[2]) = sch->GetOffsetAndLength(2, rec);
    EXPECT_EQ(off[2], 14);
    EXPECT_EQ(len[2], 2);
    EXPECT_EQ(sch->GetField(2, rec).GetInt16(), f2);

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 10);
    EXPECT_EQ(len[3], 3);
    auto f3_res = sch->GetField(3, rec);
    EXPECT_EQ(f3_res.GetVarlenAsStringView(), f3_input);

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNullableVarlenFields3) {
    TDB_TEST_BEGIN

    auto sch = GetSchemaDWithNullable();

    constexpr const uint32_t f0 = 0xfedcba98;
    constexpr absl::string_view f1_input = "100001";
    constexpr absl::string_view f3_input = "abe";

    Datum f1 = Decimal(f1_input);
    Datum f1_out = DecimalToString(f1);
    ASSERT_EQ(f1_out.GetVarlenAsStringView(), f1_input);

    Datum f3 = Varchar(f3_input);
    ASSERT_EQ(f3.GetVarlenAsStringView(), f3_input);

    maxaligned_char_buf buf;
    buf.reserve(24);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(std::move(f1)); // don't use f1 from this point
    d.emplace_back(Datum::FromNull());
    d.emplace_back(std::move(f3)); // don't use f3 from this point
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 24);
    ASSERT_EQ(buf.size(), 24);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 0);
    EXPECT_EQ(len[0], 4);
    EXPECT_EQ(sch->GetField(0, rec).GetUInt32(), f0);

    ASSERT_FALSE(sch->FieldIsNull(1, rec));
    std::tie(off[1], len[1]) = sch->GetOffsetAndLength(1, rec);
    EXPECT_EQ(off[1], 12);
    EXPECT_EQ(len[1], 8);
    auto f1_res = sch->GetField(1, rec);
    auto f1_res_out = DecimalToString(f1_res);
    EXPECT_EQ(f1_res_out.GetVarlenAsStringView(),
              f1_out.GetVarlenAsStringView());

    ASSERT_TRUE(sch->FieldIsNull(2, rec));

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 20);
    EXPECT_EQ(len[3], 3);
    auto f3_res = sch->GetField(3, rec);
    EXPECT_EQ(f3_res.GetVarlenAsStringView(), f3_input);

    TDB_TEST_END
}

TEST_F(BasicTestSchema, TestNullableVarlenFields4) {
    TDB_TEST_BEGIN

    auto sch = GetSchemaDWithNullable();

    constexpr const uint32_t f0 = 0xfedcba98;
    constexpr absl::string_view f3_input = "abe";

    Datum f3 = Varchar(f3_input);
    ASSERT_EQ(f3.GetVarlenAsStringView(), f3_input);

    maxaligned_char_buf buf;
    buf.reserve(16);

    std::vector<Datum> d;
    d.reserve(4);
    d.emplace_back(Datum::From(f0));
    d.emplace_back(Datum::FromNull());
    d.emplace_back(Datum::FromNull());
    d.emplace_back(std::move(f3)); // don't use f3 from this point
    FieldOffset reclen = sch->WritePayloadToBuffer(d, buf);
    ASSERT_EQ(reclen, 16);
    ASSERT_EQ(buf.size(), 16);
    const char* rec = buf.data();

    FieldOffset off[4];
    FieldOffset len[4];

    ASSERT_FALSE(sch->FieldIsNull(0, rec));
    std::tie(off[0], len[0]) = sch->GetOffsetAndLength(0, rec);
    EXPECT_EQ(off[0], 0);
    EXPECT_EQ(len[0], 4);
    EXPECT_EQ(sch->GetField(0, rec).GetUInt32(), f0);

    ASSERT_TRUE(sch->FieldIsNull(1, rec));

    ASSERT_TRUE(sch->FieldIsNull(2, rec));

    ASSERT_FALSE(sch->FieldIsNull(3, rec));
    std::tie(off[3], len[3]) = sch->GetOffsetAndLength(3, rec);
    EXPECT_EQ(off[3], 10);
    EXPECT_EQ(len[3], 3);
    auto f3_res = sch->GetField(3, rec);
    EXPECT_EQ(f3_res.GetVarlenAsStringView(), f3_input);

    TDB_TEST_END
}
}    // namespace taco


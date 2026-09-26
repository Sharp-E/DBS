#include "tdb.h"

#include <cctype>

#include "utils/builtin_funcs.h"

// XXX Decimal support isn't complete yet.
//
// The following input and output functions for DECIMAL only supports
// non-negative integer in base 10, and we simply the number of base-10000
// digits we'd need to represent the number, followed by that number of
// base-10000 digits.
//
// The current implementation is only used in Schema tests, and is not ready
// to be used in QP tests.

namespace taco {

namespace {

typedef uint16_t Digit;
constexpr const size_t dalign = 4;
constexpr const uint32_t maxdigits = 100000u;
constexpr const uint32_t base = 10000u;
constexpr const uint32_t nch_per_digit = 4;

struct Decimal {
    static inline size_t
    SizeOf(uint32_t ndigits) {
        return INTALIGN(offsetof(Decimal, m_digits) + sizeof(Digit) * ndigits);

    }
    static inline auto
    Create(uint32_t ndigits) {
        auto d = std::unique_ptr<Decimal, AlignedAllocImpl::FreeMem>(
            (Decimal*) aligned_alloc(dalign, SizeOf(ndigits)));
        d->m_ndigits = ndigits;
        return d;
    }

    uint16_t m_ndigits;
    Digit m_digits[1]; // more data could follow
};

}    // namespace

BUILTIN_RETTYPE(DECIMAL)
BUILTIN_FUNC(DECIMAL_in, 970)
BUILTIN_ARGTYPE(__STRING)
{
    if (FMGR_ARG(0).isnull()) {
        return Datum::FromNull();
    }
    absl::string_view in = FMGR_ARG(0).GetVarlenAsStringView();
    uint32_t i = 0;
    while (in[i] == '0' && i < in.length()) ++i;
    if (i == in.length()) {
        return Datum::FromVarlenBytes(unique_malloced_ptr(Decimal::Create(0)),
                                      Decimal::SizeOf(0));
    }

    in = in.substr(i);
    if (in.length() > maxdigits * nch_per_digit) {
        LOG(kError, "input string too long to be cast to decimal: \"%s\"",
            FMGR_ARG(0).GetVarlenAsStringView());
    }
    uint32_t ndigits = (in.length() + nch_per_digit - 1) / nch_per_digit;
    auto d = Decimal::Create(ndigits);

    auto error = [&]() {
        LOG(kError, "cannot parse \"%s\" as a DECINAL",
            FMGR_ARG(0).GetVarlenAsStringView());
    };

    uint32_t j;
    switch (in.length() % nch_per_digit) {
    case 1:
        if (!std::isdigit(in[0]))
            error();
        d->m_digits[ndigits - 1] = (Digit)(in[0] - '0');
        i = 1;
        j = ndigits - 1;
        break;

    case 2:
        if (!std::isdigit(in[0]) || !std::isdigit(in[1]))
            error();
        d->m_digits[ndigits - 1] = (in[0] - '0') * (Digit) 10 + (in[1] - '0');
        i = 2;
        j = ndigits - 1;
        break;

    case 3:
        if (!std::isdigit(in[0]) || !std::isdigit(in[1])
            || !std::isdigit(in[2]))
            error();
        d->m_digits[ndigits - 1] = (in[0] - '0') * (Digit) 100
            + (in[1] - '0') * (Digit) 10 + (in[2] - '0');
        i = 3;
        j = ndigits - 1;
        break;

    default:
        i = 0;
        j = ndigits;
    }
    for (; i < in.length(); i += nch_per_digit) {
        if (!std::isdigit(in[0]) ||
            !std::isdigit(in[1]) ||
            !std::isdigit(in[2]) ||
            !std::isdigit(in[3]))
            error();
        --j;
        d->m_digits[j] = (in[i] - '0') * (Digit) 1000 +
            (in[i + 1] - '0') * (Digit) 100 +
            (in[i + 2] - '0') * (Digit) 10 +
            (in[i + 3] - '0');
    }
    return Datum::FromVarlenBytes(unique_malloced_ptr(std::move(d)),
                                  Decimal::SizeOf(ndigits));
}


BUILTIN_RETTYPE(__STRING)
BUILTIN_FUNC(DECIMAL_out, 971)
BUILTIN_ARGTYPE(DECIMAL)
{
    if (FMGR_ARG(0).isnull())
        return Datum::FromNull();

    Decimal *d = reinterpret_cast<Decimal*>(FMGR_ARG(0).GetVarlenBytes());
    if (d->m_ndigits == 0) {
        return Datum::FromVarlenBytes("0", 1);
    }

    size_t len = d->m_ndigits * nch_per_digit;
    size_t i = d->m_ndigits - 1;
    auto t = d->m_digits[i];
    if (t < 10u) {
        len -= 3;
    } else if (t < 100u) {
        len -= 2;
    } else if (t < 1000u) {
        len -= 1;
    }

    auto p = unique_malloc(d->m_ndigits * nch_per_digit);
    char *out = (char *) p.get();
    size_t j;
    if (t < 10u) {
        out[0] = t + '0';
        j = 1;
    } else if (t < 100u) {
        out[0] = t / 10 + '0';
        out[1] = t % 10 + '0';
        j = 2;
    } else if (t < 1000u) {
        out[0] = t / 100 + '0';
        out[1] = t / 10 % 10 + '0';
        out[2] = t % 10  + '0';
        j = 3;
    } else {
        ++i;
        j = 0;
    }

    while (i > 0) {
        --i;
        t = d->m_digits[i];
        out[j] = t / 1000 + '0';
        out[j + 1] = t / 100 % 10 + '0';
        out[j + 2] = t / 10 % 10 + '0';
        out[j + 3] = t % 10 + '0';
        j += 4;
    }

    return Datum::FromVarlenBytes(std::move(p), len);
}

}    // namespace taco


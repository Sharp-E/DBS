#include "catalog/Schema.h"

#include "catalog/CatCache.h"
#include "catalog/BootstrapCatCache.h"
#include "catalog/systables.h"
#include "utils/builtin_funcs.h"

namespace taco {

static void inline
copy_bytes(bool passbyref, int16_t len, const char *src, char *tgt) {
    if (passbyref) {
        memcpy(tgt, src, len);
    } else {
        switch (len) {
        case 1:
            *(uint8_t*) tgt = *(const uint8_t*) src;
            break;
        case 2:
            *(uint16_t*) tgt = *(const uint16_t*) src;
            break;
        case 4:
            *(uint32_t*) tgt = *(const uint32_t*) src;
            break;
        default:
            *(uint64_t*) tgt = *(const uint64_t*) src;
        }
    }
}

Schema::Schema(const std::vector<Oid> &typid,
               const std::vector<uint64_t> &typparam,
               const std::vector<bool> &nullable,
               std::vector<std::string> field_names):
    m_type_info_collected(false),
    m_layout_computed(false),
    m_field(typid.empty() ? 1 : typid.size()),
    m_field_names(std::move(field_names)) {

    ASSERT(!typid.empty() && ((FieldId) typid.size()) <= MaxNumRegularFieldId);
    ASSERT(typid.size() == typparam.size());
    ASSERT(typid.size() == nullable.size());
    ASSERT(m_field_names.empty() ||
           typid.size() == m_field_names.size());

    for (FieldId i = 0; i < (FieldId) typid.size(); ++i) {
        m_field[i].m_typid = typid[i];
        m_field[i].m_typparam = typparam[i];
        m_field[i].m_nullbit_id = nullable[i] ? 0 : -1;
        m_field[i].m_offset = 0;
    }
}

Schema::Schema(const std::vector<FieldInfo>& fields,
               const std::vector<std::string>& field_names):
    m_type_info_collected(false),
    m_layout_computed(false),
    m_field(std::move(fields)),
    m_field_names(std::move(field_names)) {}

Schema*
Schema::Create(const std::vector<Oid> &typid,
               const std::vector<uint64_t> &typparam,
               const std::vector<bool> &nullable) {

    if (typid.empty() ||
        typid.size() != typparam.size() ||
        typid.size() != nullable.size()) {
        return nullptr;
    }
    return new Schema(typid, typparam, nullable, {});
}

Schema*
Schema::Combine(const Schema* left, const Schema* right) {
    std::vector<FieldInfo> fields;
    fields.reserve(left->m_field.size() + right->m_field.size());
    fields.insert(fields.end(), left->m_field.begin(), left->m_field.end());
    fields.insert(fields.end(), right->m_field.begin(), right->m_field.end());

    std::vector<std::string> field_names;
    field_names.reserve(left->m_field_names.size() + right->m_field_names.size());
    field_names.insert(field_names.end(),
                       left->m_field_names.begin(),
                       left->m_field_names.end());
    field_names.insert(field_names.end(),
                       right->m_field_names.begin(),
                       right->m_field_names.end());

    return new Schema(fields, field_names);
}

bool
Schema::Identical(const Schema* left, const Schema* right) {
    if (left->m_field.size() != right->m_field.size()) {
        return false;
    }
    for (size_t i = 0; i < left->m_field.size(); ++i) {
        if (left->m_field[i].m_typid != right->m_field[i].m_typid)
            return false;
        else if (left->m_field[i].m_typparam != right->m_field[i].m_typparam)
            return false;
        else if ((left->m_field[i].m_nullbit_id >= 0
                  && right->m_field[i].m_nullbit_id < 0) ||
                 (left->m_field[i].m_nullbit_id < 0
                  && right->m_field[i].m_nullbit_id >= 0))
            return false;
    }
    return true;
}

bool
Schema::Compatible(const Schema* left, const Schema* right) {
    if (left->m_field.size() != right->m_field.size()) {
        return false;
    }
    for (size_t i = 0; i < left->m_field.size(); ++i) {
        if (left->m_field[i].m_typid != right->m_field[i].m_typid)
            return false;
        else if (left->m_field[i].m_nullbit_id < 0
                 && right->m_field[i].m_nullbit_id >= 0)
            return false;
    }
    return true;
}

Schema*
Schema::Create(const std::vector<Oid> &typid,
               const std::vector<uint64_t> &typparam,
               const std::vector<bool> &nullable,
               std::vector<std::string> field_names) {

    if (typid.empty() ||
        typid.size() != typparam.size() ||
        typid.size() != nullable.size() ||
        (!field_names.empty() && typid.size() != field_names.size())) {
        return nullptr;
    }
    return new Schema(typid, typparam, nullable, std::move(field_names));
}

template<class CCache>
void
Schema::ComputeLayoutImpl(CCache *catcache, bool cache_typinfo_only) {
    FieldOffset off = 0;
    FieldId num_fields = GetNumFields();
    FieldId num_nonnullable_fixedlen_fields = 0;
    FieldId num_nullable_fixedlen_fields = 0;
    FieldId num_varlen_fields = 0;
    FieldId num_nullable_fields = 0;
    FieldId num_nullable_varlen_fields = 0;

    if (!cache_typinfo_only) {
        m_field_reorder_idx.resize(num_fields);
    }

    for (FieldId i = 0; i < num_fields; ++i) {
        Oid typid = m_field[i].m_typid;
        auto typ = catcache->FindType(typid);
        m_field[i].m_typlen = typ->typlen();
        m_field[i].m_typalign = typ->typalign();

        if (!typ->typisvarlen() && typ->typlenfunc() != InvalidOid) {
            FunctionInfo f = FindBuiltinFunction(typ->typlenfunc());

            Datum arg1 = Datum::From(m_field[i].m_typparam);
            Datum res = FunctionCall(f, arg1);
            ASSERT(!res.isnull());
            m_field[i].m_typlen = res.GetInt16();
        }

        if (typ->typisvarlen() || m_field[i].m_typlen == -1) {

            if (m_field[i].m_typlen != -1)
                m_field[i].m_typlen = -1;

            ASSERT(typ->typbyref());
            m_field[i].m_typbyref = true;

            if (!cache_typinfo_only) {
                ++num_varlen_fields;
                m_field[i].m_offset = -num_varlen_fields;

                if (m_field[i].m_nullbit_id >= 0) {
                    ++num_nullable_varlen_fields;
                }
            }
        } else {
            m_field[i].m_typbyref = typ->typbyref();

            if (typ->typlenfunc() != InvalidOid) {
                ASSERT(m_field[i].m_typlen >= 0);
            } else {
                ASSERT(m_field[i].m_typlen <= 8 &&
                        ((m_field[i].m_typlen - 1) & m_field[i].m_typlen) == 0);
            }

            if (!cache_typinfo_only) {
                if (m_field[i].m_nullbit_id >= 0) {
                    ++num_nullable_fixedlen_fields;
                    m_field[i].m_offset = -num_nullable_fixedlen_fields;
                } else {
                    uint8_t align = typ->typalign();
                    RETURN_IF((off = TYPEALIGN(align, off)) < 0);
                    m_field[i].m_offset = off;
                    RETURN_IF(!AddWithCheck(off, m_field[i].m_typlen));

                    m_field_reorder_idx[num_nonnullable_fixedlen_fields] = i;
                    ++num_nonnullable_fixedlen_fields;
                }
            }
        }
    }

    m_type_info_collected = true;
    if (cache_typinfo_only) {
        return ;
    }

    m_num_nonnullable_fixedlen_fields = num_nonnullable_fixedlen_fields;
    m_num_nullable_fixedlen_fields = num_nullable_fixedlen_fields;
    m_num_varlen_fields = num_varlen_fields;

    if (num_nonnullable_fixedlen_fields == num_fields) {
        RETURN_IF(MAXALIGN(off) < 0);
        off = MAXALIGN(off);
        m_null_bitmap_begin = off;
        m_varlen_end_array_begin = off;
        m_varlen_payload_begin = off;
        m_has_only_nonnullable_fixedlen_fields = true;
        m_layout_computed = true;
        return ;
    }

    m_has_only_nonnullable_fixedlen_fields = false;
    m_null_bitmap_begin = off;

    for (FieldId i = 0; i < num_fields; ++i) {
        if (m_field[i].m_typlen == -1) {
            FieldId field_seqno = num_nonnullable_fixedlen_fields +
                (-m_field[i].m_offset - 1);
            m_field_reorder_idx[field_seqno] = i;

        } else {
            if (m_field[i].m_nullbit_id >= 0) {
                FieldId field_seqno = num_nonnullable_fixedlen_fields +
                    num_varlen_fields + (-m_field[i].m_offset - 1);
                m_field_reorder_idx[field_seqno] = i;
            }
        }
        if (m_field[i].m_nullbit_id >= 0) {
            m_field[i].m_nullbit_id = num_nullable_fields;
            ++num_nullable_fields;
        }
    }

    m_null_bitmap_begin = off;
    RETURN_IF(!AddWithCheck(off, (num_nullable_fields + 7) >> 3));

    RETURN_IF((off = TYPEALIGN(sizeof(FieldOffset), off)) < 0);
    m_varlen_end_array_begin = off;
    RETURN_IF(!AddWithCheck(off,
                            sizeof(FieldOffset) * num_varlen_fields));
    m_varlen_payload_begin = off;

    m_layout_computed = true;
}

void
Schema::ComputeLayout() {
    ComputeLayoutImpl(g_db->catcache(), false);
}

void
Schema::ComputeLayout(BootstrapCatCache *catcache) {
    ComputeLayoutImpl(catcache, false);
}

void
Schema::CollectTypeInfo() {
    ComputeLayoutImpl(g_db->catcache(), true);
}

std::pair<FieldOffset, FieldOffset>
Schema::GetOffsetAndLength(
    FieldId field_id,
    const char *payload) const {
    EnsureLayoutComputed();
    const FieldInfo &f = m_field[field_id];

    if (f.m_offset >= 0) {
        return std::make_pair(f.m_offset, (FieldOffset) f.m_typlen);
    }

    const FieldOffset *varlen_end =
        (const FieldOffset *)(payload + m_varlen_end_array_begin);

    if (f.m_typlen == -1) {
        FieldId k = -f.m_offset - 1;
        FieldOffset prev_end = (k == 0) ? m_varlen_payload_begin
                                        : varlen_end[k - 1];
        FieldOffset end = varlen_end[k];
        if (FieldIsNull(field_id, payload)) {
            return std::make_pair(end, (FieldOffset) 0);
        }
        FieldOffset begin = (FieldOffset) TYPEALIGN(f.m_typalign, prev_end);
        return std::make_pair(begin, (FieldOffset)(end - begin));
    }

    FieldId k = -f.m_offset - 1;
    FieldOffset off = (m_num_varlen_fields == 0)
        ? m_varlen_payload_begin
        : varlen_end[m_num_varlen_fields - 1];
    FieldId base = m_num_nonnullable_fixedlen_fields + m_num_varlen_fields;
    for (FieldId j = 0; j < k; ++j) {
        FieldId fid = m_field_reorder_idx[base + j];
        if (!FieldIsNull(fid, payload)) {
            off = (FieldOffset) TYPEALIGN(m_field[fid].m_typalign, off);
            off += m_field[fid].m_typlen;
        }
    }
    if (FieldIsNull(field_id, payload)) {
        return std::make_pair(off, (FieldOffset) 0);
    }
    off = (FieldOffset) TYPEALIGN(f.m_typalign, off);
    return std::make_pair(off, (FieldOffset) f.m_typlen);
}

FieldOffset
Schema::WritePayloadToBuffer(const std::vector<Datum> &data,
                             maxaligned_char_buf &buf) const {
    return WritePayloadToBufferImpl(data, buf);
}

FieldOffset
Schema::WritePayloadToBuffer(const std::vector<DatumRef> &data,
                             maxaligned_char_buf &buf) const {
    return WritePayloadToBufferImpl(data, buf);
}

FieldOffset
Schema::WritePayloadToBuffer(const std::vector<NullableDatumRef> &data,
                             maxaligned_char_buf &buf) const {
    return WritePayloadToBufferImpl(data, buf);
}

template<class SomeDatum>
FieldOffset
Schema::WritePayloadToBufferImpl(const std::vector<SomeDatum> &data,
                                 maxaligned_char_buf &buf) const {
    if (buf.size() >= (size_t) std::numeric_limits<FieldOffset>::max()) {
        return -1;
    }
    FieldOffset init_len = (FieldOffset) buf.size();
    init_len = MAXALIGN(init_len);
    RETURN_IF(init_len < 0, -1);
    RETURN_IF(init_len + m_varlen_payload_begin < 0, -1);
    buf.reserve(64);
    buf.resize(init_len + m_varlen_payload_begin);

    FieldOffset off = m_varlen_payload_begin;
    char *payload_begin = buf.data() + init_len;
    for (FieldId field_id : m_field_reorder_idx) {
        FieldOffset field_len;

        const char *field_bytes;

        if (m_field[field_id].m_offset >= 0) {
            if (data[field_id].isnull()) {
                LOG(kError, "NULL value passed to non-null field "
                            FIELDID_FORMAT, field_id);
            }
            field_len = m_field[field_id].m_typlen;
            if (m_field[field_id].m_typbyref) {
                field_bytes = data[field_id].GetVarlenBytes();
                copy_bytes(true, field_len, data[field_id].GetVarlenBytes(),
                           payload_begin + m_field[field_id].m_offset);
            } else {
                field_bytes = data[field_id].GetFixedlenBytes();
                copy_bytes(false, field_len, data[field_id].GetFixedlenBytes(),
                           payload_begin + m_field[field_id].m_offset);
            }
        } else {
            if (data[field_id].isnull()) {
                char *null_bitmap = payload_begin + m_null_bitmap_begin;
                null_bitmap[m_field[field_id].m_nullbit_id >> 3] |=
                    1 << (m_field[field_id].m_nullbit_id & 7);

                if (m_field[field_id].m_typlen == -1) {
                    FieldOffset *varlen_end_array = (FieldOffset *)(
                        payload_begin + m_varlen_end_array_begin);
                    varlen_end_array[-m_field[field_id].m_offset - 1] = off;
                }
                continue;
            }

            FieldOffset newoff = TYPEALIGN(m_field[field_id].m_typalign,
                                           off);
            RETURN_IF(init_len + newoff < 0, -1);

            if (m_field[field_id].m_typlen == -1) {
                field_len = (FieldOffset) data[field_id].GetVarlenSize();
                field_bytes = data[field_id].GetVarlenBytes();

                FieldOffset *varlen_end_array = (FieldOffset *) (
                    payload_begin + m_varlen_end_array_begin);
                varlen_end_array[-m_field[field_id].m_offset - 1] =
                    newoff + field_len;
            } else {
                field_len = m_field[field_id].m_typlen;
                if (m_field[field_id].m_typbyref) {
                    field_bytes = data[field_id].GetVarlenBytes();
                } else {
                    field_bytes = data[field_id].GetFixedlenBytes();
                }
            }
            RETURN_IF(init_len + newoff + field_len < 0, -1);

            ASSERT(off + init_len == (FieldOffset) buf.size());
            buf.resize(newoff + field_len + init_len);
            payload_begin = buf.data() + init_len;
            copy_bytes(m_field[field_id].m_typbyref,
                       field_len, field_bytes,
                       payload_begin + newoff);
            off = newoff + field_len;
        }
    }

    ASSERT(off + init_len == (FieldOffset) buf.size());
    off = MAXALIGN(off);
    RETURN_IF(init_len + off < 0, -1);
    if (off + init_len > (FieldOffset) buf.size()) {
        buf.resize(off + init_len);
    }
    return off;
}

bool
Schema::FieldIsNull(FieldId field_id, const char *payload) const {
    EnsureLayoutComputed();
    FieldId nullbit = m_field[field_id].m_nullbit_id;
    if (nullbit < 0) {
        return false;
    }
    const unsigned char *bitmap =
        (const unsigned char *)(payload + m_null_bitmap_begin);
    return (bitmap[nullbit >> 3] >> (nullbit & 7)) & 1;
}

Datum
Schema::GetField(FieldId field_id, const char *payload) const {
    if (FieldIsNull(field_id, payload)) {
        return Datum::FromNull();
    }

    auto p = GetOffsetAndLength(field_id, payload);
    if (m_field[field_id].m_typlen == -1) {
        return Datum::FromVarlenBytes(payload + p.first, p.second);
    }

    if (m_field[field_id].m_typbyref) {
        return Datum::FromVarlenBytes(payload + p.first,
                                      m_field[field_id].m_typlen);
    }

    return Datum::FromFixedlenBytes(payload + p.first, p.second);
}

std::vector<Datum>
Schema::DissemblePayload(const char *payload) const {
    std::vector<Datum> ret;
    FieldId n = GetNumFields();
    for (FieldId i = 0; i < n; ++i) {
        ret.emplace_back(GetField(i, payload));
    }
    return ret;
}

FieldId
Schema::GetFieldIdFromFieldName(absl::string_view field_name) const {
    FieldId n = (FieldId) m_field_names.size();
    for (FieldId i = 0; i < n; ++i) {
        if (field_name == m_field_names[i])
            return i;
    }
    return InvalidFieldId;
}

}

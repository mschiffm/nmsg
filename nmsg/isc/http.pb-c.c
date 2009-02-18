/* Generated by the protocol buffer compiler.  DO NOT EDIT! */

#include "http.pb-c.h"
void   nmsg__isc__http__init
                     (Nmsg__Isc__Http         *message)
{
  static Nmsg__Isc__Http init_value = NMSG__ISC__HTTP__INIT;
  *message = init_value;
}
size_t nmsg__isc__http__get_packed_size
                     (const Nmsg__Isc__Http *message)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &nmsg__isc__http__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t nmsg__isc__http__pack
                     (const Nmsg__Isc__Http *message,
                      uint8_t       *out)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &nmsg__isc__http__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t nmsg__isc__http__pack_to_buffer
                     (const Nmsg__Isc__Http *message,
                      ProtobufCBuffer *buffer)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &nmsg__isc__http__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Nmsg__Isc__Http *
       nmsg__isc__http__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Nmsg__Isc__Http *)
     protobuf_c_message_unpack (&nmsg__isc__http__descriptor,
                                allocator, len, data);
}
void   nmsg__isc__http__free_unpacked
                     (Nmsg__Isc__Http *message,
                      ProtobufCAllocator *allocator)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &nmsg__isc__http__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor nmsg__isc__http__field_descriptors[18] =
{
  {
    "type",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_ENUM,
    0,   /* quantifier_offset */
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, type),
    &nmsg__isc__http_type__descriptor,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "srcip",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_srcip),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, srcip),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "srchost",
    3,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_srchost),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, srchost),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "srcport",
    4,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_srcport),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, srcport),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "dstip",
    5,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_dstip),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, dstip),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "dstport",
    6,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_dstport),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, dstport),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "request",
    7,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_request),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, request),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "p0f_genre",
    65,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_p0f_genre),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, p0f_genre),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "p0f_detail",
    66,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_p0f_detail),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, p0f_detail),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "p0f_dist",
    67,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_INT32,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_p0f_dist),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, p0f_dist),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "p0f_link",
    68,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_p0f_link),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, p0f_link),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "p0f_tos",
    69,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_p0f_tos),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, p0f_tos),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "p0f_fw",
    70,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_p0f_fw),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, p0f_fw),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "p0f_nat",
    71,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_p0f_nat),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, p0f_nat),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "p0f_real",
    72,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_p0f_real),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, p0f_real),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "p0f_score",
    73,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_INT32,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_p0f_score),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, p0f_score),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "p0f_mflags",
    74,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_p0f_mflags),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, p0f_mflags),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
  {
    "p0f_uptime",
    75,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_INT32,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, has_p0f_uptime),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Http, p0f_uptime),
    NULL,
    NULL,
    NULL,NULL    /* reserved1, reserved2 */
  },
};
static const unsigned nmsg__isc__http__field_indices_by_name[] = {
  4,   /* field[4] = dstip */
  5,   /* field[5] = dstport */
  8,   /* field[8] = p0f_detail */
  9,   /* field[9] = p0f_dist */
  12,   /* field[12] = p0f_fw */
  7,   /* field[7] = p0f_genre */
  10,   /* field[10] = p0f_link */
  16,   /* field[16] = p0f_mflags */
  13,   /* field[13] = p0f_nat */
  14,   /* field[14] = p0f_real */
  15,   /* field[15] = p0f_score */
  11,   /* field[11] = p0f_tos */
  17,   /* field[17] = p0f_uptime */
  6,   /* field[6] = request */
  2,   /* field[2] = srchost */
  1,   /* field[1] = srcip */
  3,   /* field[3] = srcport */
  0,   /* field[0] = type */
};
static const ProtobufCIntRange nmsg__isc__http__number_ranges[2 + 1] =
{
  { 1, 0 },
  { 65, 7 },
  { 0, 18 }
};
const ProtobufCMessageDescriptor nmsg__isc__http__descriptor =
{
  PROTOBUF_C_MESSAGE_DESCRIPTOR_MAGIC,
  "nmsg.isc.Http",
  "Http",
  "Nmsg__Isc__Http",
  "nmsg.isc",
  sizeof(Nmsg__Isc__Http),
  18,
  nmsg__isc__http__field_descriptors,
  nmsg__isc__http__field_indices_by_name,
  2,  nmsg__isc__http__number_ranges,
  NULL,NULL,NULL,NULL    /* reserved[1234] */
};
const ProtobufCEnumValue nmsg__isc__http_type__enum_values_by_number[2] =
{
  { "unknown", "NMSG__ISC__HTTP_TYPE__UNKNOWN", 0 },
  { "sinkhole", "NMSG__ISC__HTTP_TYPE__SINKHOLE", 1 },
};
static const ProtobufCIntRange nmsg__isc__http_type__value_ranges[] = {
{0, 0},{0, 2}
};
const ProtobufCEnumValueIndex nmsg__isc__http_type__enum_values_by_name[2] =
{
  { "sinkhole", 1 },
  { "unknown", 0 },
};
const ProtobufCEnumDescriptor nmsg__isc__http_type__descriptor =
{
  PROTOBUF_C_ENUM_DESCRIPTOR_MAGIC,
  "nmsg.isc.HttpType",
  "HttpType",
  "Nmsg__Isc__HttpType",
  "nmsg.isc",
  2,
  nmsg__isc__http_type__enum_values_by_number,
  2,
  nmsg__isc__http_type__enum_values_by_name,
  1,
  nmsg__isc__http_type__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};

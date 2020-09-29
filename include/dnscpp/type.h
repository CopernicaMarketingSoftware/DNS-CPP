/**
 *  Type.h
 * 
 *  All the DNS record types
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include <arpa/nameser.h>

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  The record types
 *  @var int
 */
const ns_type TYPE_A           = (ns_type)1;
const ns_type TYPE_NS          = (ns_type)2;
const ns_type TYPE_MD          = (ns_type)3;
const ns_type TYPE_MF          = (ns_type)4;
const ns_type TYPE_CNAME       = (ns_type)5;
const ns_type TYPE_SOA         = (ns_type)6;
const ns_type TYPE_MB          = (ns_type)7;
const ns_type TYPE_MG          = (ns_type)8;
const ns_type TYPE_MR          = (ns_type)9;
const ns_type TYPE_NULL        = (ns_type)10;
const ns_type TYPE_WKS         = (ns_type)11;
const ns_type TYPE_PTR         = (ns_type)12;
const ns_type TYPE_HINFO       = (ns_type)13;
const ns_type TYPE_MINFO       = (ns_type)14;
const ns_type TYPE_MX          = (ns_type)15;
const ns_type TYPE_TXT         = (ns_type)16;
const ns_type TYPE_RP          = (ns_type)17;
const ns_type TYPE_AFSDB       = (ns_type)18;
const ns_type TYPE_X25         = (ns_type)19;
const ns_type TYPE_ISDN        = (ns_type)20;
const ns_type TYPE_RT          = (ns_type)21;
const ns_type TYPE_NSAP        = (ns_type)22;
const ns_type TYPE_NSAP_PTR    = (ns_type)23;
const ns_type TYPE_SIG         = (ns_type)24;
const ns_type TYPE_KEY         = (ns_type)25;
const ns_type TYPE_PX          = (ns_type)26;
const ns_type TYPE_GPOS        = (ns_type)27;
const ns_type TYPE_AAAA        = (ns_type)28;
const ns_type TYPE_LOC         = (ns_type)29;
const ns_type TYPE_NXT         = (ns_type)30;
const ns_type TYPE_EID         = (ns_type)31;
const ns_type TYPE_NIMLOC      = (ns_type)32;
const ns_type TYPE_SRV         = (ns_type)33;
const ns_type TYPE_ATMA        = (ns_type)34;
const ns_type TYPE_NAPTR       = (ns_type)35;
const ns_type TYPE_KX          = (ns_type)36;
const ns_type TYPE_CERT        = (ns_type)37;
const ns_type TYPE_A6          = (ns_type)38;
const ns_type TYPE_DNAME       = (ns_type)39;
const ns_type TYPE_SINK        = (ns_type)40;
const ns_type TYPE_OPT         = (ns_type)41;
const ns_type TYPE_APL         = (ns_type)42;
const ns_type TYPE_DS          = (ns_type)43;
const ns_type TYPE_SSHFP       = (ns_type)44;
const ns_type TYPE_IPSECKEY    = (ns_type)45;
const ns_type TYPE_RRSIG       = (ns_type)46;
const ns_type TYPE_NSEC        = (ns_type)47;
const ns_type TYPE_DNSKEY      = (ns_type)48;
const ns_type TYPE_DHCID       = (ns_type)49;
const ns_type TYPE_NSEC3       = (ns_type)50;
const ns_type TYPE_NSEC3PARAM  = (ns_type)51;
const ns_type TYPE_TLSA        = (ns_type)52;
const ns_type TYPE_SMIMEA      = (ns_type)53;
const ns_type TYPE_HIP         = (ns_type)55;
const ns_type TYPE_NINFO       = (ns_type)56;
const ns_type TYPE_RKEY        = (ns_type)57;
const ns_type TYPE_TALINK      = (ns_type)58;
const ns_type TYPE_CDS         = (ns_type)59;
const ns_type TYPE_CDNSKEY     = (ns_type)60;
const ns_type TYPE_OPENPGPKEY  = (ns_type)61;
const ns_type TYPE_CSYNC       = (ns_type)62;
const ns_type TYPE_SPF         = (ns_type)99;
const ns_type TYPE_UINFO       = (ns_type)100;
const ns_type TYPE_UID         = (ns_type)101;
const ns_type TYPE_GID         = (ns_type)102;
const ns_type TYPE_UNSPEC      = (ns_type)103;
const ns_type TYPE_NID         = (ns_type)104;
const ns_type TYPE_L32         = (ns_type)105;
const ns_type TYPE_L64         = (ns_type)106;
const ns_type TYPE_LP          = (ns_type)107;
const ns_type TYPE_EUI48       = (ns_type)108;
const ns_type TYPE_EUI64       = (ns_type)109;
const ns_type TYPE_TKEY        = (ns_type)249;
const ns_type TYPE_TSIG        = (ns_type)250;
const ns_type TYPE_IXFR        = (ns_type)251;
const ns_type TYPE_AXFR        = (ns_type)252;
const ns_type TYPE_MAILB       = (ns_type)253;
const ns_type TYPE_MAILA       = (ns_type)254;
const ns_type TYPE_ANY         = (ns_type)255;
const ns_type TYPE_URI         = (ns_type)256;
const ns_type TYPE_CAA         = (ns_type)257;
const ns_type TYPE_AVC         = (ns_type)258;
const ns_type TYPE_TA          = (ns_type)32768;
const ns_type TYPE_DLV         = (ns_type)32769;

/**
 *  End of namespace
 */
}

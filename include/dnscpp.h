/**
 *  DnsCpp.h
 * 
 *  Main include file for the dnscpp library
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Other dependencies
 */
#include <dnscpp/context.h>
#include <dnscpp/loop.h>
#include <dnscpp/handler.h>
#include <dnscpp/response.h>
#include <dnscpp/query.h>
#include <dnscpp/answer.h>
#include <dnscpp/a.h>
#include <dnscpp/cname.h>
#include <dnscpp/aaaa.h>
#include <dnscpp/mx.h>
#include <dnscpp/txt.h>
#include <dnscpp/caa.h>
#include <dnscpp/ns.h>
#include <dnscpp/ptr.h>
#include <dnscpp/soa.h>
#include <dnscpp/rrsig.h>
#include <dnscpp/dnskey.h>
#include <dnscpp/printable.h>
#include <dnscpp/hosts.h>
#include <dnscpp/operation.h>
#include <dnscpp/request.h>
#include <dnscpp/question.h>

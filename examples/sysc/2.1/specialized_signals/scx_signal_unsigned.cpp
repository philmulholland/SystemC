/*****************************************************************************

  Licensed to Accellera Systems Initiative Inc. (Accellera) under one or
  more contributor license agreements.  See the NOTICE file distributed
  with this work for additional information regarding copyright ownership.
  Accellera licenses this file to you under the Apache License, Version 2.0
  (the "License"); you may not use this file except in compliance with the
  License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
  implied.  See the License for the specific language governing
  permissions and limitations under the License.

 *****************************************************************************/

/*****************************************************************************

  sc_signal_unsigned.cpp -- The sc_signal<sc_unsigned<W> > implementations.

  Original Author: Andy Goodrich, Forte Design Systems, 2002-10-22

 *****************************************************************************/

/*****************************************************************************

  MODIFICATION LOG - modifiers, enter your name, affiliation, date and
  changes you are making here.

      Name, Affiliation, Date:
  Description of Modification:

 *****************************************************************************/

/*
$Log: scx_signal_unsigned.cpp,v $
Revision 1.3  2011/08/26 22:28:29  acg
 Torsten Maehne: eliminate unused argument warnings.

Revision 1.2  2011/08/15 16:43:24  acg
 Torsten Maehne: changes to remove unused argument warnings.

Revision 1.1.1.1  2006/12/15 20:20:03  acg
SystemC 2.3

Revision 1.2  2005/12/26 20:11:14  acg
Fixed up copyright.

Revision 1.1.1.1  2005/12/19 23:16:42  acg
First check in of SystemC 2.1 into its own archive.

Revision 1.17  2005/09/15 23:01:52  acg
Added std:: prefix to appropriate methods and types to get around
issues with the Edison Front End.

Revision 1.16  2005/05/03 19:52:26  acg
Get proper header locations on includes.

Revision 1.15  2005/05/03 19:50:20  acg
Name space version.

Revision 1.12  2005/04/11 19:05:36  acg
Change to sc_clock for Microsoft VCC 6.0. Changes for namespaces

Revision 1.11  2005/04/03 22:52:52  acg
Namespace changes.

Revision 1.10  2005/03/21 22:31:33  acg
Changes to sc_core namespace.

Revision 1.9  2004/09/27 21:01:59  acg
Andy Goodrich - Forte Design Systems, Inc.
   - This is specialized signal support that allows better use of signals
     and ports whose target value is a SystemC native type.

*/



#include "sysc/utils/sc_temporary.h"
#include "sysc/communication/sc_interface.h"
#include "sysc/communication/sc_port.h"
#include "sysc/communication/sc_prim_channel.h"
#include "sysc/communication/sc_signal_ports.h"
#include "sysc/communication/sc_signal.h"
#include "sysc/kernel/sc_event.h"
#include "sysc/kernel/sc_process.h"
#include "sysc/kernel/sc_simcontext.h"
#include "sysc/kernel/sc_macros.h"
#include "sysc/tracing/sc_trace.h"
#include "sysc/datatypes/int/sc_nbdefs.h"
#include "sysc/datatypes/misc/sc_concatref.h"
#include "sysc/datatypes/int/sc_int.h"
#include "sysc/datatypes/int/sc_uint.h"
#include "sysc/datatypes/int/sc_signed.h"
#include "sysc/datatypes/int/sc_unsigned.h"
#include "sysc/datatypes/int/sc_biguint.h"
#include "sysc/datatypes/bit/sc_lv_base.h"

#include "./scx_signal_unsigned.h"
#include <typeinfo>

namespace sc_core {

extern
void
sc_signal_invalid_writer( const char* name,
                          const char* kind,
                          const char* first_writer,
                          const char* second_writer );

//------------------------------------------------------------------------------
// POOL OF TEMPORARY INSTANCES OF sc_unsigned_sigref
//
// This allows use to pass const references for part and bit selections
// on sc_signal_signed object instances.
//------------------------------------------------------------------------------
sc_vpool<sc_unsigned_sigref> sc_unsigned_sigref::m_pool(8);


//------------------------------------------------------------------------------
//"sc_unsigned_part_if::default methods"
//
// These versions just produce errors if they are not overloaded but used.
//------------------------------------------------------------------------------
sc_dt::sc_unsigned* sc_unsigned_part_if::part_read_target()
{
    SC_REPORT_ERROR( SC_ID_OPERATION_ON_NON_SPECIALIZED_SIGNAL_, "int" );
    return 0;
}
sc_dt::sc_unsigned sc_unsigned_part_if::read_part( int /*left*/, int /*right*/ ) const
{
    SC_REPORT_ERROR( SC_ID_OPERATION_ON_NON_SPECIALIZED_SIGNAL_, "int" );
    sc_core::sc_abort(); // can't recover from here
}
sc_unsigned_sigref& sc_unsigned_part_if::select_part(int /*left*/, int /*right*/)
{
    SC_REPORT_ERROR( SC_ID_OPERATION_ON_NON_SPECIALIZED_SIGNAL_, "int" );
    sc_core::sc_abort(); // can't recover from here
}
void sc_unsigned_part_if::write_part( sc_dt::int64 /*v*/, int /*left*/, int /*right*/ )
{
    SC_REPORT_ERROR( SC_ID_OPERATION_ON_NON_SPECIALIZED_SIGNAL_, "int" );
}
void sc_unsigned_part_if::write_part( sc_dt::uint64 /*v*/, int /*left*/, int /*right*/ )
{
    SC_REPORT_ERROR( SC_ID_OPERATION_ON_NON_SPECIALIZED_SIGNAL_, "int" );
}
void sc_unsigned_part_if::write_part(
    const sc_dt::sc_signed& /*v*/, int /*left*/, int /*right*/ )
{
    SC_REPORT_ERROR( SC_ID_OPERATION_ON_NON_SPECIALIZED_SIGNAL_, "int" );
}
void sc_unsigned_part_if::write_part(
    const sc_dt::sc_unsigned& /*v*/, int /*left*/, int /*right*/ )
{
    SC_REPORT_ERROR( SC_ID_OPERATION_ON_NON_SPECIALIZED_SIGNAL_, "int" );
}



//------------------------------------------------------------------------------
//"sc_unsigned_sigref::concate_set"
//
// These methods assign this object instance's value from the supplied
// value starting at the supplied bit within that value.
//     src = value to use to set this object instance's value.
//     low_i = bit in src that is to be the low order bit of the value to set.
// #### OPTIMIZE
//------------------------------------------------------------------------------
void sc_unsigned_sigref::concat_set(sc_dt::int64 src, int low_i)
{
    sc_dt::int64 tmp;
    if ( low_i < 63 )
        tmp = src >> low_i;
    else
        tmp = (src < 0) ? -1 : 0;
    m_if_p->write_part( tmp, m_left, m_right );
}


void sc_unsigned_sigref::concat_set(const sc_dt::sc_signed& src, int low_i)
{
    m_if_p->write_part( src >> low_i, m_left, m_right );
}


void sc_unsigned_sigref::concat_set(const sc_dt::sc_lv_base& src, int low_i)
{
    sc_dt::sc_unsigned tmp(src.length());
    tmp = src;
    m_if_p->write_part( tmp >> low_i, m_left, m_right );
}


void sc_unsigned_sigref::concat_set(const sc_dt::sc_unsigned& src, int low_i)
{
    m_if_p->write_part( src >> low_i, m_left, m_right );
}


void sc_unsigned_sigref::concat_set(sc_dt::uint64 src, int low_i)
{
    sc_dt::uint64 tmp = (low_i < 63) ? (src >> low_i) : 0;
    m_if_p->write_part( tmp, m_left, m_right );
}


} // namespace sc_core

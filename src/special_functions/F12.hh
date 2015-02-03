//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   sf/F12.hh
 * \author Kent Budge
 * \brief  Compute Fermi-Dirac function of 1/2 order
 * \note   Copyright (C) 2004-2015 Los Alamos National Security, LLC.
 *         All rights reserved.
 */
//---------------------------------------------------------------------------//
// $Id$
//---------------------------------------------------------------------------//

#ifndef sf_F12_hh
#define sf_F12_hh

#include "ds++/config.h"

namespace rtt_sf
{
//! Calculate Fermi-Dirac integral of index 1/2.
DLL_PUBLIC double F12(double x);

} // end namespace rtt_sf

#endif // sf_F12_hh

//---------------------------------------------------------------------------//
// end of sf/F12.hh
//---------------------------------------------------------------------------//

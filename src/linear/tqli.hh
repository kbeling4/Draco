//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   linear/tqli.hh
 * \author Kent Budge
 * \date   Thu Sep  2 15:00:32 2004
 * \brief  Find eigenvectors and eigenvalues of a symmetric matrix that
 *         has been reduced to tridiagonal form via a call to tred2. 
 * \note   Copyright (C) 2004-2015 Los Alamos National Security, LLC.
 *         All rights reserved.
 */
//---------------------------------------------------------------------------//
// $Id$
//---------------------------------------------------------------------------//

#ifndef linear_tqli_hh
#define linear_tqli_hh

namespace rtt_linear
{
//! Find eigenvectors and eigenvalues of a symmetric matrix that has been reduced to tridiagonal form via a call to rtt_linear::tred2. 

template<class FieldVector1, class FieldVector2, class FieldVector3>
void tqli(FieldVector1 &d,
	  FieldVector2 &e,
	  const unsigned n,
	  FieldVector3 &z);

} // end namespace rtt_linear

#endif // linear_tqli_hh

//---------------------------------------------------------------------------//
// end of linear/tqli.hh
//---------------------------------------------------------------------------//

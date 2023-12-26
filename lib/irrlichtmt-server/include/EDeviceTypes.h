// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __E_DEVICE_TYPES_H_INCLUDED__
#define __E_DEVICE_TYPES_H_INCLUDED__

namespace irr
{

	//! An enum for the different device types supported by the Irrlicht Engine.
	enum E_DEVICE_TYPE
	{

		//! A device native to Unix style operating systems.
		/** This device uses the X11 windowing system and works in Linux, Solaris, FreeBSD, OSX and
		other operating systems which support X11. */
		EIDT_X11,

		//! This selection allows Irrlicht to choose the best device from the ones available.
		/** If this selection is chosen then Irrlicht will try to use the IrrlichtDevice native
		to your operating system. If this is unavailable then the X11, SDL and then console device
		will be tried. This ensures that Irrlicht will run even if your platform is unsupported,
		although it may not be able to render anything. */
		EIDT_BEST,
	};

} // end namespace irr

#endif // __E_DEVICE_TYPES_H_INCLUDED__


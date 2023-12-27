/*
Minetest
Copyright (C) 2015, 2016 est31 <MTest31@outlook.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

/// Gets the base64 encoded legacy password db entry.
String translate_password(const String &name,
	const String &password);

/// Creates a verification key with given salt and password.
String generate_srp_verifier(const String &name,
	const String &password, const String &salt);

/// Creates a verification key and salt with given password.
void generate_srp_verifier_and_salt(const String &name,
	const String &password, String *verifier,
	String *salt);

/// Gets an SRP verifier, generating a salt,
/// and encodes it as DB-ready string.
String get_encoded_srp_verifier(const String &name,
	const String &password);

/// Converts the passed SRP verifier into a DB-ready format.
String encode_srp_verifier(const String &verifier,
	const String &salt);

/// Reads the DB-formatted SRP verifier and gets the verifier
/// and salt components.
bool decode_srp_verifier_and_salt(const String &encoded,
	String *verifier, String *salt);

/**
 * Copyright (c) 2006-2026 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

// Forked and edited by J01nK Productions (2020-2026)

#include "wrap_DataModule.h"
#include "wrap_Data.h"
#include "wrap_ByteData.h"
#include "wrap_DataView.h"
#include "wrap_CompressedData.h"
#include "DataModule.h"
#include "common/b64.h"
#include "common/lua.hpp"

#include <cmath>
#include <iostream>
#include <algorithm>
#include <limits>

namespace love
{
namespace data
{

#define instance() (Module::getInstance<DataModule>(Module::M_DATA))

ContainerType luax_checkcontainertype(lua_State *L, int idx)
{
	const char *str = luaL_checkstring(L, idx);
	ContainerType ctype = CONTAINER_STRING;
	if (!getConstant(str, ctype))
		luax_enumerror(L, "container type", getConstants(ctype), str);
	return ctype;
}

int w_newDataView(lua_State *L)
{
	Data *data = luax_checkdata(L, 1);

	lua_Integer offset = luaL_checkinteger(L, 2);
	lua_Integer size   = luaL_optinteger(L, 3, (lua_Integer)data->getSize() - offset);

	if (offset < 0 || size < 0)
		return luaL_error(L, "DataView offset and size must not be negative.");

	DataView *d;
	luax_catchexcept(L, [&]() { d = instance()->newDataView(data, (size_t)offset, (size_t)size); });
	luax_pushtype(L, d);
	d->release();

	return 1;
}

int w_newByteData(lua_State *L)
{
	ByteData *d = nullptr;

	if (luax_istype(L, 1, Data::type))
	{
		Data *data = luax_checkdata(L, 1);

		if (data->getSize() > (size_t)std::numeric_limits<lua_Integer>::max())
			return luaL_error(L, "Data's size is too large!");

		lua_Integer offset = luaL_optinteger(L, 2, 0);
		if (offset < 0)
			return luaL_error(L, "Offset argument must not be negative.");

		lua_Integer size = luaL_optinteger(L, 3, (lua_Integer)data->getSize() - offset);
		if (size <= 0)
			return luaL_error(L, "Size argument must be greater than zero.");
		else if ((size_t)(offset + size) > data->getSize())
			return luaL_error(L, "Offset and size arguments must fit within the given Data's size.");

		const char *bytes = (const char *)data->getData() + offset;
		luax_catchexcept(L, [&]() { d = instance()->newByteData(bytes, (size_t)size); });
	}
	else if (lua_type(L, 1) == LUA_TSTRING)
	{
		size_t      sz  = 0;
		const char *src = luaL_checklstring(L, 1, &sz);
		luax_catchexcept(L, [&]() { d = instance()->newByteData(src, sz); });
	}
	else
	{
		lua_Integer size = luaL_checkinteger(L, 1);
		if (size <= 0)
			return luaL_error(L, "Data size must be a positive number.");
		luax_catchexcept(L, [&]() { d = instance()->newByteData((size_t)size); });
	}

	luax_pushtype(L, d);
	d->release();
	return 1;
}

int w_compress(lua_State *L)
{
	ContainerType ctype = luax_checkcontainertype(L, 1);

	const char        *fstr   = luaL_checkstring(L, 2);
	Compressor::Format format = Compressor::FORMAT_LZ4;

	if (!Compressor::getConstant(fstr, format))
		return luax_enumerror(L, "compressed data format", Compressor::getConstants(format), fstr);

	int         level    = (int)luaL_optinteger(L, 4, -1);
	size_t      rawsize  = 0;
	const char *rawbytes = nullptr;

	if (lua_isstring(L, 3))
		rawbytes = luaL_checklstring(L, 3, &rawsize);
	else
	{
		Data *rawdata = luax_checktype<Data>(L, 3);
		rawsize  = rawdata->getSize();
		rawbytes = (const char *)rawdata->getData();
	}

	CompressedData *cdata = nullptr;
	luax_catchexcept(L, [&]() { cdata = compress(format, rawbytes, rawsize, level); });

	if (ctype == CONTAINER_DATA)
		luax_pushtype(L, cdata);
	else
		lua_pushlstring(L, (const char *)cdata->getData(), cdata->getSize());

	cdata->release();
	return 1;
}

int w_decompress(lua_State *L)
{
	ContainerType ctype = luax_checkcontainertype(L, 1);

	char  *rawbytes = nullptr;
	size_t rawsize  = 0;

	if (luax_istype(L, 2, CompressedData::type))
	{
		CompressedData *data = luax_checkcompresseddata(L, 2);
		rawsize = data->getDecompressedSize();
		luax_catchexcept(L, [&]() { rawbytes = decompress(data, rawsize); });
	}
	else
	{
		Compressor::Format format = Compressor::FORMAT_LZ4;
		const char        *fstr   = luaL_checkstring(L, 2);

		if (!Compressor::getConstant(fstr, format))
			return luax_enumerror(L, "compressed data format", Compressor::getConstants(format), fstr);

		size_t      compressedsize = 0;
		const char *cbytes         = nullptr;

		if (luax_istype(L, 3, Data::type))
		{
			Data *data     = luax_checktype<Data>(L, 3);
			cbytes         = (const char *)data->getData();
			compressedsize = data->getSize();
		}
		else
			cbytes = luaL_checklstring(L, 3, &compressedsize);

		luax_catchexcept(L, [&]() { rawbytes = decompress(format, cbytes, compressedsize, rawsize); });
	}

	if (ctype == CONTAINER_DATA)
	{
		ByteData *data = nullptr;
		luax_catchexcept(L, [&]() { data = instance()->newByteData(rawbytes, rawsize, true); });
		luax_pushtype(L, Data::type, data);
		data->release();
	}
	else
	{
		lua_pushlstring(L, rawbytes, rawsize);
		delete[] rawbytes;
	}

	return 1;
}

int w_encode(lua_State *L)
{
	ContainerType ctype = luax_checkcontainertype(L, 1);

	const char *formatstr = luaL_checkstring(L, 2);
	EncodeFormat format;
	if (!getConstant(formatstr, format))
		return luax_enumerror(L, "encode format", getConstants(format), formatstr);

	size_t      srclen = 0;
	const char *src    = nullptr;

	if (luax_istype(L, 3, Data::type))
	{
		Data *data = luax_totype<Data>(L, 3);
		src    = (const char *)data->getData();
		srclen = data->getSize();
	}
	else
		src = luaL_checklstring(L, 3, &srclen);

	size_t linelen = (size_t)luaL_optinteger(L, 4, 0);

	size_t dstlen = 0;
	char  *dst    = nullptr;
	luax_catchexcept(L, [&]() { dst = encode(format, src, srclen, dstlen, linelen); });

	if (ctype == CONTAINER_DATA)
	{
		ByteData *data = nullptr;
		if (dst != nullptr)
			luax_catchexcept(L, [&]() { data = instance()->newByteData(dst, dstlen, true); });
		else
			luax_catchexcept(L, [&]() { data = instance()->newByteData((size_t)0); });

		luax_pushtype(L, Data::type, data);
		data->release();
	}
	else
	{
		if (dst != nullptr)
			lua_pushlstring(L, dst, dstlen);
		else
			lua_pushstring(L, "");

		delete[] dst;
	}

	return 1;
}

int w_decode(lua_State *L)
{
	ContainerType ctype = luax_checkcontainertype(L, 1);

	const char *formatstr = luaL_checkstring(L, 2);
	EncodeFormat format;
	if (!getConstant(formatstr, format))
		return luax_enumerror(L, "decode format", getConstants(format), formatstr);

	size_t      srclen = 0;
	const char *src    = nullptr;

	if (luax_istype(L, 3, Data::type))
	{
		Data *data = luax_totype<Data>(L, 3);
		src    = (const char *)data->getData();
		srclen = data->getSize();
	}
	else
		src = luaL_checklstring(L, 3, &srclen);

	size_t dstlen = 0;
	char  *dst    = nullptr;
	luax_catchexcept(L, [&]() { dst = decode(format, src, srclen, dstlen); });

	if (ctype == CONTAINER_DATA)
	{
		ByteData *data = nullptr;
		if (dst != nullptr)
			luax_catchexcept(L, [&]() { data = instance()->newByteData(dst, dstlen, true); });
		else
			luax_catchexcept(L, [&]() { data = instance()->newByteData((size_t)0); });

		luax_pushtype(L, Data::type, data);
		data->release();
	}
	else
	{
		if (dst != nullptr)
			lua_pushlstring(L, dst, dstlen);
		else
			lua_pushstring(L, "");

		delete[] dst;
	}

	return 1;
}

int w_hash(lua_State *L)
{
	ContainerType          ctype   = CONTAINER_STRING;
	HashFunction::Function function;
	int                    dataarg = 3;

	const char *str = luaL_checkstring(L, 1);

	if (!getConstant(str, ctype))
	{
		if (HashFunction::getConstant(str, function))
		{
			luax_markdeprecated(L, 1, "love.data.hash", API_FUNCTION_VARIANT, DEPRECATED_REPLACED,
			                    "variant with container return type parameter");
			dataarg = 2;
		}
		else
		{
			return luax_enumerror(L, "container type", getConstants(ctype), str);
		}
	}
	else
	{
		const char *fstr = luaL_checkstring(L, 2);
		if (!HashFunction::getConstant(fstr, function))
			return luax_enumerror(L, "hash function", HashFunction::getConstants(function), fstr);
	}

	HashFunction::Value hashvalue;
	if (lua_isstring(L, dataarg))
	{
		size_t      rawsize  = 0;
		const char *rawbytes = luaL_checklstring(L, dataarg, &rawsize);
		luax_catchexcept(L, [&]() { love::data::hash(function, rawbytes, rawsize, hashvalue); });
	}
	else
	{
		Data *rawdata = luax_checktype<Data>(L, dataarg);
		luax_catchexcept(L, [&]() { love::data::hash(function, rawdata, hashvalue); });
	}

	if (ctype == CONTAINER_DATA)
	{
		Data *d = nullptr;
		luax_catchexcept(L, [&]() { d = instance()->newByteData(hashvalue.size); });
		memcpy(d->getData(), hashvalue.data, hashvalue.size);
		luax_pushtype(L, Data::type, d);
		d->release();
	}
	else
		lua_pushlstring(L, hashvalue.data, hashvalue.size);

	return 1;
}

// ---------------------------------------------------------------------------
// w_pack
//
// lua53_str_pack(L) is a void macro taking exactly 1 argument.
// It reads:  stack[1] = format string,  stack[2..N] = values to pack
// and pushes the resulting packed binary string onto the top of the stack.
//
// To drive it from C we copy the needed args into a fresh contiguous window
// at the top of the stack using lua_pushvalue, then call the macro, then
// read the result string back with lua_tolstring.
//
// ByteData path  (Lua args: bytedata[1], offset[2], fmt[3], v1[4], ...)
// Normal path    (Lua args: ctype[1],    fmt[2],    v1[3], ...)
// ---------------------------------------------------------------------------

int w_pack(lua_State *L)
{
	if (luax_istype(L, 1, ByteData::type))
	{
		ByteData   *d      = luax_checkbytedata(L, 1);
		size_t      offset = (size_t)luaL_checknumber(L, 2);
		int         top    = lua_gettop(L);

		// Push fmt (index 3) then values (index 4..top) onto the top of the stack.
		lua_pushvalue(L, 3);
		for (int i = 4; i <= top; i++)
			lua_pushvalue(L, i);

		// Stack is now: original[1..top], fmt, v1, v2, ...
		// Strip the original args from indices 1..top so that the copies
		// we just pushed become indices 1..copy_count.
		for (int i = 0; i < top; i++)
			lua_remove(L, 1);

		lua53_str_pack(L);

		size_t      packed_size = 0;
		const char *packed_data = lua_tolstring(L, -1, &packed_size);

		if (offset + packed_size > d->getSize())
		{
			lua_pop(L, 1);
			return luaL_error(L, "The given byte offset and pack format parameters do not fit within the ByteData's size.");
		}

		memcpy((uint8 *)d->getData() + offset, packed_data, packed_size);
		lua_pop(L, 1);

		luax_pushtype(L, Data::type, d);
		return 1;
	}

	// Normal path: ctype[1], fmt[2], v1[3], ...
	ContainerType ctype = luax_checkcontainertype(L, 1);
	int           top   = lua_gettop(L);

	// Copy fmt and values onto the top of the stack, then strip the originals.
	lua_pushvalue(L, 2);
	for (int i = 3; i <= top; i++)
		lua_pushvalue(L, i);

	for (int i = 0; i < top; i++)
		lua_remove(L, 1);

	lua53_str_pack(L);

	size_t      packed_size = 0;
	const char *packed_data = lua_tolstring(L, -1, &packed_size);

	if (ctype == CONTAINER_DATA)
	{
		Data *d = nullptr;
		luax_catchexcept(L, [&]() { d = instance()->newByteData(packed_size); });
		memcpy(d->getData(), packed_data, packed_size);
		lua_pop(L, 1);
		luax_pushtype(L, Data::type, d);
		d->release();
	}

	// CONTAINER_STRING: the packed string is already on top of the stack.
	return 1;
}

// ---------------------------------------------------------------------------
// w_unpack
//
// lua53_str_unpack(L) is a void macro taking exactly 1 argument.
// It reads:  stack[1] = format,  stack[2] = data string,  stack[3] = pos (opt)
// and pushes unpacked values + next-byte-position integer onto the stack.
//
// If arg 2 came in as a Data object we materialise the raw string onto the
// stack at position 2 before calling the macro.
// ---------------------------------------------------------------------------

int w_unpack(lua_State *L)
{
	luaL_checkstring(L, 1);

	if (luax_istype(L, 2, Data::type))
	{
		Data       *d    = luax_checkdata(L, 2);
		const char *data = (const char *)d->getData();
		size_t      sz   = d->getSize();
		lua_pushlstring(L, data, sz);
		lua_replace(L, 2);
	}
	else
		luaL_checkstring(L, 2);

	// Stack is now: fmt[1], data_string[2], [pos[3]]
	int before = lua_gettop(L);
	lua53_str_unpack(L);
	return lua_gettop(L) - before;
}

// ---------------------------------------------------------------------------
// w_getPackedSize
//
// lua53_str_packsize does not exist in this bridge.
// We delegate to Lua's string.packsize at runtime.
// ---------------------------------------------------------------------------

int w_getPackedSize(lua_State *L)
{
	luaL_checkstring(L, 1);

	lua_getglobal(L, "string");
	lua_getfield(L, -1, "packsize");
	lua_remove(L, -2);

	if (!lua_isfunction(L, -1))
		return luaL_error(L, "string.packsize is not available in this runtime.");

	lua_pushvalue(L, 1);
	lua_call(L, 1, 1);
	return 1;
}

static const luaL_Reg functions[] =
{
	{ "newDataView",   w_newDataView   },
	{ "newByteData",   w_newByteData   },
	{ "compress",      w_compress      },
	{ "decompress",    w_decompress    },
	{ "encode",        w_encode        },
	{ "decode",        w_decode        },
	{ "hash",          w_hash          },
	{ "pack",          w_pack          },
	{ "unpack",        w_unpack        },
	{ "getPackedSize", w_getPackedSize },
	{ 0, 0 }
};

static const lua_CFunction types[] =
{
	luaopen_data,
	luaopen_bytedata,
	luaopen_dataview,
	luaopen_compresseddata,
	nullptr
};

extern "C" int luaopen_love_data(lua_State *L)
{
	DataModule *instance = instance();
	if (instance == nullptr)
		luax_catchexcept(L, [&]() { instance = new DataModule(); });
	else
		instance->retain();

	WrappedModule w;
	w.module    = instance;
	w.name      = "data";
	w.type      = &Module::type;
	w.functions = functions;
	w.types     = types;

	return luax_register_module(L, w);
}

} // data
} // love
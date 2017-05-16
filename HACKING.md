# Hacking sickd

## Philosophy

sickd is designed to be small, lightweight. It should do the very minimum to get
data from a Sick laser sensor, and export it to other processes. sickd is
designed to be portable and compatible with the latest C standard. To that
extent, sickd only uses standard C features, or libraries that are portable
across a wide range of systems. It is also perfectly fine to use POSIX features
that have no equivalent in the standard C libraries.

## Coding style

sickd uses the Linux kernel coding style.  sickd also uses several stricter
rules which are outlined below.

### stdint.h vs shortint types

Always use standard C99 types, such as uint32_t, uint8_t, etc. Do not reinvent
types as shortint types (u32, u8, etc).


### Never use 'unsigned' without a type specifier

Always use 'unsigned' with another type specifier. For example 'unsigned int',
or 'unsigned long'. Don't try to be clever and save space here. something like
'unsigned variable;'

### Passing error information

Always pass error information as a negative integer. Do not re-invent an enum
of "error codes we might encounter". Those usually end up being too vague or
incomplete to pass any useful information other that a simple pass/fail.
Instead return POSIX error codes in errno.h:

	if (sp_get_port_by_name(port, port_name) != SP_OK)
		return -ENODEV;

However, never set errno! Always use the return value. When a dependent library
does not return descriptive error codes, but provides a facility to return the
system error code, use that facility. For libserialport this is
'sp_last_error_code()'.

In functions that are supposed to return a pointer to something, return NULL on
failures. Do not try to encode the error code in the return pointer!

### Only use 'char' to represent ASCII strings

Do not use 'char' or 'unsigned char' to represent bytes in a data stream. 'char'
should only be used to store ASCII 'char'acters. For binary streams use
'uint8_t' or 'void *'.

### Representation and decoding of binary streams

In the context of sickd, binary streams are streams of data arriving from some
external source or leaving sickd to some external source. While streams may
contain packets, they are not considered datagrams until decoded into packets.

#### Passing around of streams

Binary streams should always be represented as 'uint8_t *' or 'void *', with an
associated 'size_t len' element. It is acceptable to omit the length element
in cases when the length has been previously checked, and the called function is
known to only access at most 'len' bytes from the stream. This generally means
that the consumer of the stream is a function with static linkage.
The following example shows an acceptable use of streams without a len argument:

	static int num_points(uint8_t *packet)
	{
		return (int)packet[8] * packet[9];
	}

	int process_something(void *data, size_t len)
	{
		int num_pts;

		if (len < MIN_PACKET_LEN)
			return 0;

		num_pts = num_points(data);
		...
	}

#### Streams as 'uint8_t *' vs "void *'

Generally, streams should be passed as 'void *'. However, in cases where data in
the stream needs to be accessed, it is preferred to pass the stream as a
'uint8_t *'. This allows accessing individual bytes in the stream.

Do not perform any arithmetic on 'void *' streams. Instead, use 'uint8_t *', or
cast to 'uintptr_t'.

#### Accessing data in streams

Data in streams should be accessed and decoded byte-wise. Do not cast the
stream to a type other than 'uint8_t *', and try to dereference it. For example
castind a stream to 'uint32_t *'and dereferencing it is a big no-no. Use
helpers such as read_le32() for this task.

Also, do not use signed types on a stream. For example, do not cast to
'int8_t *' and assume that the host will magically absorb the correct
representation. If the stream contains a signed number in two's complement
representation, then _decode_ it as such. Do not cast it. For example:

	int offset;

	/* Two's complement, signed 8-bit integer */
	offset = data[0] & 0x7f;
	if (data[0] & (1 << 7))
		offset -= 128

## Documentation

Documentation should be written in a format that is human-readable in plaintext.
The same 80-character limit that applies to code also applies to the
documentation. Plaintext is okay.  Markdown is preferred. HTML is a big no-no!

Formats designed to be compiled into pretty documentation, such as LaTeX, are
acceptable provided that the information cannot be efficiently conveyed in a
plaintext format.

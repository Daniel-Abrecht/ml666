# ml666 semantics

This is about the format and how tokenizers & parsers are to treat it in general. For details about the implementation
provided in this repo, see [ml666.md](ml666.md).

The syntax assumes each character is an 8 bit byte. For the values of characters in the ABNF syntax, ASCII is used.
The decoded contents of all tokens is binary data. But if a program chooses to interpret any of them as text, it
must encode/decode it using utf8 itself, and it must verify the validity of the unicode data itself.

Take a look at the [syntax.abnf](syntax.abnf) file for the syntax. The tokens the tokenizer should emit are indicated in uppercase.
The `document` token matches the whole ml666 file. The tokenizer may choose to reuse the `END_TAG` token instead of
having a seperate `SELFCLOSING` token. This makes parsing easier. The `END_TAG` token should be either empty, or match
the `TAG` token of the corresponding `element` token. The tokenizer doesn't have to check this, but the parser should do so.
The content of the returned tokens should not contain any raw escape sequences, instead, the tokenizer should interpret
them when it encounters them. A tokenizer may return a token in a streaming fashion, it may return them as chunks, or as
a whole, this is left to the implementation.

There are additional special semantics for string tokens & multiline comments,
the tokenizer shall apply the following additional transformations:

* For `plain_string` and `multiline_comment`: If the first character is a newline, remove it. If the last line only contains `SP` " " characters,
  and is precided but not followed by a newline, remove that line, including the preceding newline. The string / comment
  delimiters  `` / /**/ / //  should not be included in returned token content.
* For `hex_string` and `base64_string`: The parser should decode the content as hex or base64 correspondingly, and return the result as a token.
  The `base64_string` may also contain base64url encoded content, the decoding has to be relaxed to that end. Spaces can be anywhere and do not
  affect the en/decoding. Note that the ABNF syntax does not permit incomplete base64url or hex strings.


A tokenizer or parser may or may not choose to emit comments. It is also up to the implementation, if different adjacent strings where allowed are
merged into a single token or not, or even further split up. An implementation may or may not provide a means determine if a string or comment token
is already complete, and it may or may not provide a means to determine the encoding the string had.

A tokenizer must provide a means to determine if an attribute is complete, because there can be multiple adjacent value-less attributes.

# ML666 semantics

This is about the format and how tokenizers & parsers are to treat it in general. For details about the implementation
provided in this repo, see [ml666.md](ml666.md).

The formal syntax assumes each character is an 8 bit byte. For the values of characters in the ABNF syntax, ASCII is used.
However, the encoded ml666 document itself must be valid, normalized utf-8 data without any utf-16 surrogates or BOMs,
and without any noncharacter codepoints, with the exception of the optional (and *not* recommended) utf8 BOM.
Any program reading an ml666 document must validate that the document meets these criteria.
Only a certain amount of UTF-8 code pages are currently used, so there exist valid UTF-8 sequences for not yet valid
unicode characters. A parser may or may not reject those. If it does, it needs to be updated when more unicode codepoints
become valid. A serializer must always escape such sequences, to ensure all parsers will the ml666 document.

The decoded contents of all tokens is binary data. But if a program chooses to interpret any of them as text, it
must encode/decode it using utf8 itself, and it must verify the validity of the decoded unicode data itself.

Take a look at the [syntax.abnf](syntax.abnf) file for the syntax. The tokens the tokenizer should emit are indicated in uppercase.
The `document` token matches the whole ml666 file. The tokenizer may choose to reuse the `END_TAG` token instead of
having a seperate `SELFCLOSING` token. This makes parsing easier. The `END_TAG` token should be either empty, or match
the `TAG` token of the corresponding `element` token. The tokenizer doesn't have to check this, but the parser should do so.
The content of the returned tokens should not contain any raw escape sequences, instead, the tokenizer should interpret
them when it encounters them. A tokenizer may return a token in a streaming fashion, it may return them as chunks, or as
a whole, this is left to the implementation.

There are additional special semantics for string tokens & comments,
the tokenizer shall apply the following additional transformations:

* The string / comment delimiters  `` / /**/ / //  should not be included in the returned token content. But, the final newline of a single line
  comment should be part of the token content.
* If there is a space at the very beginning of a single line comment, it should be removed.
* For `plain_string` and `multiline_comment`: If the first character is a newline, remove it. If the last line only contains `SP` " " characters,
  and is precided but not followed by a newline, remove that line, including the preceding newline.
* For a `multiline_comment`, if the first or last character is a space, it should not be part of the token content.
* For `hex_string` and `base64_string`: The parser should decode the content as hex or base64 correspondingly, and return the result as a token.
  The `base64_string` may also contain base64url encoded content, the decoding has to be relaxed to that end. Spaces can be anywhere and do not
  affect the en/decoding. Note that the ML666 syntax does not permit incomplete base64url or hex strings.


A tokenizer or parser may or may not choose to emit comments. It is also up to the implementation, if different adjacent strings where allowed are
merged into a single token or not, or even further split up. An implementation may or may not provide a means determine if a string or comment token
is already complete, and it may or may not provide a means to determine the encoding the string had.

A tokenizer must provide a means to determine if an attribute is complete, because there can be multiple adjacent value-less attributes.

ArcaneText — Smart Mapping Text Obfuscator
What is this?

ArcaneText is a tool to encode and decode text files using a custom byte-to-byte mapping, defined by the user. It replaces each character’s byte in your text with a user-configurable sequence of bytes (often hex codes). This transforms readable text into an obfuscated byte sequence, and vice versa, based on your mapping.

Example: Included! 

main_custom.cpp is the same as main.cpp, but its custom mapped using ArcaneText + a custom mapping. I am not going to provide the correct mapping,
so take a swing at it and try to get it back to normal if you want. This provides a good example of how ArcaneText works in my opinion.


The project includes a GTK UI to load/save text files, generate or edit the mapping, and save/load the mapping as JSON.
What does it do?

    Allows you to map every character (byte) in your text to an arbitrary hex byte sequence.

    Encodes your text files by substituting each character with the mapped sequence.

    Decodes obfuscated files back to the original text using the reverse mapping.

    Lets you import/export the mapping as JSON for reuse or sharing.

    Provides a graphical UI to edit and manage mappings interactively.

What this is not

    This is not encryption. It does not provide cryptographic security or strong protection against a determined attacker.

    The mapping can be reverse-engineered or brute-forced if someone knows or guesses the structure.

    It’s an obfuscation tool designed to make data unreadable via a transformation that renders data incomprehensible without the correct mapping, but it does not guarantee confidentiality or integrity.

    It is vulnerable to analysis if patterns are visible, or if the mapping leaks.

How does it work?

    User defines a mapping: each byte (character) maps to a byte sequence (often a single hex byte).

    Encoding walks through the text, replacing each byte with its mapped sequence.

    Decoding reverses this using the inverse of the mapping.

    The UI helps create and adjust mappings, load/save mappings from JSON, and handle file encoding/decoding.

Use cases

    Basic obfuscation of text files to avoid casual reading.

    Sharing data publicly with your team who have the mapping file.

    Experimentation with custom byte substitution.

    Educational tool to explore byte-level text transformations.

Limitations

    Anyone with the mapping file or enough samples can decode the content.

    Does not defend against known plaintext attacks.

    Not suitable for sensitive or critical data security.

    Performance depends on mapping complexity and file size.

Dependencies

    GTK 3 for the UI.

    json-c for JSON parsing and writing.

    Standard C++17 for codebase.

Building & Running

Build with your favorite C++ compiler and GTK 3 development libraries installed. For example:

g++ -std=c++17 arcane_text.cpp -o arcane_text `pkg-config --cflags --libs gtk+-3.0 json-c`
./arcane_text

Summary

ArcaneText is a user-driven byte substitution tool for obfuscating text. It trades simplicity and control for no real security. Use it for light obfuscation or private data-sharing scenarios where strong encryption is not required.

If you want help generating example mappings or usage instructions, I can provide those too.

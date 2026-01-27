/*
  dumpBuffer.ino

  Display the contents of a buffer in hexadecimal and ASCII

  License: MIT. Please see LICENSE.md for more details
*/

//----------------------------------------
// Display the contents of a buffer in hexadecimal and ASCII
//
// Inputs:
//   buffer: Address of a buffer containing the data to display
//   length: Number of bytes of data to display
//----------------------------------------
void dumpBuffer(const uint8_t *buffer, size_t length)
{
    int bytes;
    const uint8_t *end;
    int index;
    size_t offset;

    end = &buffer[length];
    offset = 0;
    while (buffer < end)
    {
        // Determine the number of bytes to display on the line
        bytes = end - buffer;
        if (bytes > (16 - (offset & 0xf)))
            bytes = 16 - (offset & 0xf);

        // Display the offset
        sempPrintHex0x08x(output, offset);
        sempPrintString(output, ": ");

        // Skip leading bytes
        for (index = 0; index < (offset & 0xf); index++)
            sempPrintString(output, "   ");

        // Display the data bytes
        for (index = 0; index < bytes; index++)
        {
            sempPrintHex02x(output, buffer[index]);
            output(' ');
        }

        // Separate the data bytes from the ASCII
        for (; index < (16 - (offset & 0xf)); index++)
            sempPrintString(output, "   ");
        sempPrintString(output, " ");

        // Skip leading bytes
        for (index = 0; index < (offset & 0xf); index++)
            sempPrintString(output, " ");

        // Display the ASCII values
        for (index = 0; index < bytes; index++)
            output(((buffer[index] < ' ') || (buffer[index] >= 0x7f)) ? '.' : buffer[index]);
        sempPrintLn(output);

        // Set the next line of data
        buffer += bytes;
        offset += bytes;
    }
}

void flipHV(GContext *ctx) {
  GBitmap *fb = graphics_capture_frame_buffer(ctx);  //  Get Screen image
  GRect fbb = gbitmap_get_bounds(fb);  //  ... and it's dimensions

  int Format = gbitmap_get_format(fb);  //  Get the Color format
  int ByteWidth = gbitmap_get_bytes_per_row(fb);  //  Get image width in bytes
  int ImageWidth = fbb.size.w;   //  Get image width in pixels
  int ImageHeight = fbb.size.h;  // Get image height in pixels

//  Calculte image byte width
  switch (Format) {
    case GBitmapFormat1Bit:
      ImageWidth = ImageWidth / 8;
      break;
  #ifdef PBL_PLATFORM_BASALT
    case GBitmapFormat1BitPalette:
      ImageWidth = ImageWidth / 8;
      break;
    case GBitmapFormat2BitPalette:
      ImageWidth = ImageWidth / 4;
      break;
    case GBitmapFormat4BitPalette:
      ImageWidth = ImageWidth / 2;
      break;
  #endif
  }

//  Calculate unused bytes needed to make up to a number divisible by 4
  int Overhang = ByteWidth - ImageWidth;

  uint8_t *pBase = gbitmap_get_data(fb);  //  Get image start address
  uint8_t *pTopRemainingPixel = pBase;    //  Set address of first pixel
//  Set address of last pixel taking account of overhanging bytes
  uint8_t *pBottomRemainingPixel = pBase + (ImageHeight * ByteWidth) - Overhang - 1;

  int CurrentCol = 0;  //  Set current column (-1) being worked on
  while (pTopRemainingPixel < pBottomRemainingPixel) {  //  Until all pixels have been swapped

    CurrentCol++;  //  Increment Column being worked on

    uint8_t TopPixel = *pTopRemainingPixel;  //  Get current top pixel value
    uint8_t BottomPixel = *pBottomRemainingPixel;  //  Get current bottom pixel value
    if (TopPixel != BottomPixel) {  //  Only need to do anything if they are not the same
      switch (Format) {  //  Reverse bits according to Color Palette
        case 0:  //  GBitmapFormat1Bit
        case 2:  //  GBitmapFormat1BitPalette
          TopPixel = (TopPixel & 0xF0) >> 4 | (TopPixel & 0x0F) << 4;
          BottomPixel = (BottomPixel & 0xF0) >> 4 | (BottomPixel & 0x0F) << 4;
        case 3:  // GBitmapFormat2BitPalette
          TopPixel = (TopPixel & 0xCC) >> 2 | (TopPixel & 0x33) << 2;
          BottomPixel = (BottomPixel & 0xCC) >> 2 | (BottomPixel & 0x33) << 2;
        case 4:  //  GBitmapFormat4BitPalette
          TopPixel = (TopPixel & 0xAA) >> 1 | (TopPixel & 0x55) << 1;
          BottomPixel = (BottomPixel & 0xAA) >> 1 | (BottomPixel & 0x55) << 1;
      }
      *pBottomRemainingPixel = TopPixel;  //  Set new current top pixel
      *pTopRemainingPixel = BottomPixel;  //  Set new current bottom pixel
    }
    pTopRemainingPixel++;     //  Increment to next top pixel
    pBottomRemainingPixel--;  //  Decrement to next bottom pixel

//  If we are at the end of the actual image row, skip any overhang bytes to the start of the nest row
    if (CurrentCol >= ImageWidth) {
      CurrentCol = 0;
      pTopRemainingPixel += Overhang;
      pBottomRemainingPixel -= Overhang;
    }
  }
  graphics_release_frame_buffer(ctx,fb);  //  Relase the frame buffer
}

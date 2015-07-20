void flipHV(GContext *ctx) {
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  GRect fbb = gbitmap_get_bounds(fb);
  int Width =  fbb.size.w;
  int Height = fbb.size.h;

  uint8_t *pBase = gbitmap_get_data(fb);
  uint8_t *pTopRemainingPixel = pBase;
  uint8_t *pBottomRemainingPixel = pBase + (Height * Width);

  while (pTopRemainingPixel < pBottomRemainingPixel) {
    uint8_t TopPixel = *pTopRemainingPixel;
    uint8_t BottomPixel = *pBottomRemainingPixel;
    if (TopPixel != BottomPixel) {
      *pBottomRemainingPixel = TopPixel;
      *pTopRemainingPixel = BottomPixel;
    }
    pTopRemainingPixel++;
    pBottomRemainingPixel--;
  }
  graphics_release_frame_buffer(ctx,fb);
}

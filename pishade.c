#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <termios.h>
#include "bcm_host.h"
#include "element_change.h"

// Round x up to the next multiple of y.  dispmanx_vnc uses this same
// macro and the author commented in the source code that Dispmanx
// expects buffer rows to be aligned to 32-bit boundaries.
// see: https://www.raspberrypi.org/forums/viewtopic.php?f=67&t=140855
#ifndef ALIGN_UP
#define ALIGN_UP(x,y)  ((x + (y)-1) & ~((y)-1))
#endif

static void FillRect(void *image, int pitch, int x, int y, int w, int h,
                     int val)
{
    int row;
    int col;

    uint16_t *line = (uint16_t *)image + y * (pitch>>1) + x;

    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col++) {
            line[col] = val;
        }
        line += (pitch>>1);
    }
}

int main(void)
{
  DISPMANX_DISPLAY_HANDLE_T display;
  DISPMANX_MODEINFO_T info;
  uint32_t screen = 0;
  int width = 200, height = 200;
  int ret;
  bcm_host_init();

  display = vc_dispmanx_display_open(screen);
  assert(display != 0);

  ret = vc_dispmanx_display_get_info(display, &info);
  assert(ret == 0);

  printf("Display is %dx%d\n", info.width, info.height);

  width = info.width;
  height = info.height;

  // Allocate some memory for our square.
  void *image;
  int pitch = ALIGN_UP(width*2, 32);
  image = calloc(1, pitch * height);
  assert(image);

  // Fill the memory with our green square.
  /* FillRect(image, pitch, 0, 0, width, height, 0xf800); */
  FillRect(image, pitch, 0, 0, width, height, 0x0000);

  // Create the resource to which our element will refer.
  DISPMANX_RESOURCE_HANDLE_T resource;
  uint32_t vc_image_ptr;
  resource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, width, height,
                                         &vc_image_ptr);
  assert(resource);

  // Define the rectangle into which we will draw.
  VC_RECT_T rect;
  vc_dispmanx_rect_set(&rect, 0, 0, width, height);

  // Write out the data.
  ret = vc_dispmanx_resource_write_data(resource, VC_IMAGE_RGB565, pitch,
                                        image, &rect);
  assert(ret == 0);

  // Now we draw
  DISPMANX_UPDATE_HANDLE_T update;
  update = vc_dispmanx_update_start(10);
  assert(update);

  VC_RECT_T src_rect;
  vc_dispmanx_rect_set(&src_rect, 0, 0, width << 16, height << 16);

  vc_dispmanx_rect_set(&rect, 0, 0, width, height);

  DISPMANX_ELEMENT_HANDLE_T element;
  VC_DISPMANX_ALPHA_T alpha = {
    DISPMANX_FLAGS_ALPHA_FROM_SOURCE |
    DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 255, /*alpha 0->255*/ 0
  };
  element = vc_dispmanx_element_add(update,
                                    display,
                                    2000,               // layer
                                    &rect,
                                    resource,
                                    &src_rect,
                                    DISPMANX_PROTECTION_NONE,
                                    &alpha,
                                    NULL,             // clamp
                                    VC_IMAGE_ROT0);

  ret = vc_dispmanx_update_submit_sync(update);
  assert(ret == 0);

  // START OPACITY
  char my_char = 0;
  struct termios original;
  tcgetattr(STDIN_FILENO, &original);

  // Copy the termios attributes so we can modify them.
  struct termios term;
  memcpy(&term, &original, sizeof(term));

  // Switch to noncanonical mode and turn off
  term.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &term);

  uint8_t new_alpha = 255;
  while ((my_char = getchar()) != 'q') {
    // printf("%c", my_char);

    switch (my_char) {
    case 'j':
      new_alpha -= 5;
      break;
    case 'k':
      new_alpha += 5;
      break;
    default:
      continue;
    }

    update = vc_dispmanx_update_start(10);
    assert(update);
    ret = vc_dispmanx_element_change_attributes(update,
                                                element,
                                                ELEMENT_CHANGE_OPACITY,
                                                2000,
                                                new_alpha,
                                                &rect,
                                                &src_rect,
                                                0,
                                                DISPMANX_NO_ROTATE);
    assert(ret == 0);
    ret = vc_dispmanx_update_submit_sync(update);
    assert(ret == 0);
  }

  // Restore original terminal attributes
  tcsetattr(STDIN_FILENO, TCSANOW, &original);

  // END OPACITY

  /* printf("Sleeping for 10 seconds...\n"); */
  /* sleep(10); */

  update = vc_dispmanx_update_start(10);
  assert(update);
  ret = vc_dispmanx_element_remove(update, element);
  assert(ret == 0);
  ret = vc_dispmanx_update_submit_sync(update);
  assert(ret == 0);
  ret = vc_dispmanx_resource_delete(resource);
  assert(ret == 0);

  ret = vc_dispmanx_display_close(display);
  assert(ret == 0);

  return 0;
}

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <libgimp/gimp.h>

static void query(void);
static void run(const gchar *name, gint nparams, const GimpParam *param,
                gint *nreturn_vals, GimpParam **return_vals);

GimpPlugInInfo PLUG_IN_INFO = {
    NULL,  /* init_procedure */
    NULL,  /* quit_procedure */
    query, /* query_procedure */
    run,   /* run_procedure */
};

MAIN()

static void query(void) {
  static const GimpParamDef load_args[] = {
      {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
      {GIMP_PDB_STRING, "filename", "The name of the file to load"},
      {GIMP_PDB_STRING, "raw_filename", "The name of the file to load"},
  };
  static const GimpParamDef load_return_vals[] = {
      {GIMP_PDB_IMAGE, "image", "Output image"},
  };

  gimp_install_procedure("file_farbfeld_load",
          "Loads farbfeld images",
          "Loads farbfeld images.",
          "Ian D. Scott",
          "Copyright Ian D. Scott",
          "2016",
          "farbfeld image",
          NULL,
          GIMP_PLUGIN,
          G_N_ELEMENTS(load_args),
          G_N_ELEMENTS(load_return_vals),
          load_args,
          load_return_vals);

  gimp_register_load_handler("file_farbfeld_load", "farbfeld", "");
}

static void run(const gchar *name, gint nparams, const GimpParam *param,
                gint *nreturn_vals, GimpParam **return_vals) {
  static GimpParam values[2];

  *return_vals = values;

  if (!strcmp(name, "file_farbfeld_load")) {
    char *filename;
    gint32 image, layer;
    filename = param[1].data.d_string;
    uint16_t height, width, i, j, k;
    FILE *file;
    uint8_t hdr[strlen("farbfeld") + 2 * sizeof(uint32_t)];
    uint16_t rgba[4];
    guchar *buf;
    GimpDrawable *drawable;
    GimpPixelRgn pixel_region;

    file = fopen(filename, "rb");
    fread(hdr, 1, sizeof(hdr), file);
    width = ntohl(*((uint32_t *)(hdr + 8)));
    height = ntohl(*((uint32_t *)(hdr + 12)));

    image = gimp_image_new(width, height, GIMP_RGB);
    gimp_image_set_filename(image, filename);
    layer = gimp_layer_new(image, "Background", width, height, GIMP_RGBA_IMAGE,
                           100.0, GIMP_NORMAL_MODE);
    gimp_image_insert_layer(image, layer, 0, 0);
    drawable = gimp_drawable_get(layer);
    gimp_pixel_rgn_init(&pixel_region, drawable, 0, 0, width,
                        height, TRUE, FALSE);

    buf = malloc(height * width * 4);

    for (i = 0; i < height; i++) {
      for (j = 0; j < width; j++) {
        fread(rgba, sizeof(uint16_t), 4, file);
        for (k = 0; k < 4; k++)
          buf[i*width*4 + j*4 + k] = ntohs(rgba[k]) / 257;
      }
    }

    gimp_pixel_rgn_set_rect(&pixel_region, buf, 0, 0, width, height);
    free(buf);
    gimp_drawable_flush(drawable);
    gimp_drawable_detach(drawable);

    *nreturn_vals = 2;
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = GIMP_PDB_SUCCESS;
    values[1].type = GIMP_PDB_IMAGE;
    values[1].data.d_image = image;

  } else {
    *nreturn_vals = 1;
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
  }
}

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bzlib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <libgimp/gimp.h>

#include "config.h"

#define LOAD_PROC "file_farbfeld_load"
#define SAVE_PROC "file_farbfeld_save"

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

    static const GimpParamDef save_args[] = {
        {GIMP_PDB_INT32, "run-mode", "Interactive, non-interactive"},
        {GIMP_PDB_IMAGE, "image", "Input image"},
        {GIMP_PDB_DRAWABLE, "drawable", "Drawable to save"},
        {GIMP_PDB_STRING, "filename", "The name of the file to save"},
        {GIMP_PDB_STRING, "raw-filename", "The name of the file to save"}};

    static const GimpParamDef load_return_vals[] = {
        {GIMP_PDB_IMAGE, "image", "Output image"},
    };

    gimp_install_procedure(LOAD_PROC,
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

    gimp_register_file_handler_mime(LOAD_PROC, "image/farbfeld");
    gimp_register_magic_load_handler(
        LOAD_PROC, extensions, "", "0,string,farbfeld");

    gimp_install_procedure(SAVE_PROC,
                           "Saves farbfeld images",
                           "Saves farbfeld images.",
                           "Ian D. Scott",
                           "Copyright Ian D. Scott",
                           "2016",
                           "farbfeld image",
                           "RGB",
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(save_args),
                           0,
                           save_args,
                           NULL);

    gimp_register_file_handler_mime(SAVE_PROC, "image/farbfeld");
    gimp_register_save_handler(SAVE_PROC, extensions, "");
}

static void run(const gchar *name, gint nparams, const GimpParam *param,
                gint *nreturn_vals, GimpParam **return_vals) {
    static GimpParam values[2];
    char *filename, *ext;
    FILE *file;
    uint8_t hdr[strlen("farbfeld") + 2 * sizeof(uint32_t)];
    uint16_t rgba[4], height, width, i, j, k, bpp;
    guchar *buf;
    gint32 image, layer;
    GimpDrawable *drawable;
    GimpImageType image_type;
    GimpPixelRgn pixel_region;

    *return_vals = values;

    if (strcmp(name, LOAD_PROC) == 0) {
        filename = param[1].data.d_string;
        file = g_fopen(filename, "rb");

        fread(hdr, 1, sizeof(hdr), file);

        width = ntohl(*((uint32_t *)(hdr + 8)));
        height = ntohl(*((uint32_t *)(hdr + 12)));

        image = gimp_image_new(width, height, GIMP_RGB);
        gimp_image_set_filename(image, filename);
        layer = gimp_layer_new(image,
                               _("Background"),
                               width,
                               height,
                               GIMP_RGBA_IMAGE,
                               100.0,
                               GIMP_NORMAL_MODE);
        gimp_image_insert_layer(image, layer, 0, 0);
        drawable = gimp_drawable_get(layer);
        gimp_pixel_rgn_init(
            &pixel_region, drawable, 0, 0, width, height, TRUE, FALSE);

        buf = malloc(height * width * 4);

        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                fread(rgba, sizeof(uint16_t), 4, file);
                for (k = 0; k < 4; k++)
                    buf[i * width * 4 + j * 4 + k] = ntohs(rgba[k]) / 257;
            }
        }

        fclose(file);
        gimp_pixel_rgn_set_rect(&pixel_region, buf, 0, 0, width, height);
        free(buf);
        gimp_drawable_flush(drawable);
        gimp_drawable_detach(drawable);

        *nreturn_vals = 2;
        values[0].type = GIMP_PDB_STATUS;
        values[0].data.d_status = GIMP_PDB_SUCCESS;
        values[1].type = GIMP_PDB_IMAGE;
        values[1].data.d_image = image;
    } else if (strcmp(name, SAVE_PROC) == 0) {
        image = param[1].data.d_int32;
        drawable = gimp_drawable_get(param[2].data.d_int32);
        filename = param[3].data.d_string;

        image_type = gimp_drawable_type(drawable->drawable_id);
        if ((image_type != GIMP_RGBA_IMAGE) && (image_type != GIMP_RGB_IMAGE)) {
            *nreturn_vals = 2;
            values[0].type = GIMP_PDB_STATUS;
            values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
            values[1].type = GIMP_PDB_STRING;
            values[1].data.d_string =
                "Image must be RGB/RGBA for farbfeld export.";
            return;
        }

        width = drawable->width;
        height = drawable->height;
        gimp_pixel_rgn_init(
            &pixel_region, drawable, 0, 0, width, height, FALSE, FALSE);

        file = g_fopen(filename, "wb");
        ext = strrchr(filename, '.');

        memcpy(hdr, "farbfeld", strlen("farbfeld"));
        *((uint32_t *)(hdr + 8)) = htonl(width);
        *((uint32_t *)(hdr + 12)) = htonl(height);
        fwrite(hdr, sizeof(hdr), 1, file);

        bpp = (image_type == GIMP_RGB_IMAGE) ? 3 : 4;
        buf = malloc(height * width * bpp);
        gimp_pixel_rgn_get_rect(&pixel_region, buf, 0, 0, width, height);

        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                for (k = 0; k < bpp; k++)
                    rgba[k] = htons(buf[i * width * bpp + j * bpp + k] * 257);
                rgba[3] = (bpp < 4) ? 255 : rgba[3];
                fwrite(rgba, sizeof(uint16_t), 4, file);
            }
        }

        free(buf);
        fclose(file);
        gimp_drawable_detach(drawable);

        *nreturn_vals = 1;
        values[0].type = GIMP_PDB_STATUS;
        values[0].data.d_status = GIMP_PDB_SUCCESS;

    } else {
        *nreturn_vals = 1;
        values[0].type = GIMP_PDB_STATUS;
        values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
    }
}

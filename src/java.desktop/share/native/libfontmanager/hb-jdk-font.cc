/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include "hb.h"
#include "hb-jdk.h"
#ifdef MACOSX
#include "hb-coretext.h"
#endif
#include <stdlib.h>

#if defined(__GNUC__) &&  __GNUC__ >= 4
#define HB_UNUSED       __attribute__((unused))
#else
#define HB_UNUSED
#endif


static hb_bool_t
hb_jdk_get_nominal_glyph (hb_font_t *font HB_UNUSED,
		          void *font_data,
		          hb_codepoint_t unicode,
		          hb_codepoint_t *glyph,
		          void *user_data HB_UNUSED)
{

    JDKFontInfo *jdkFontInfo = (JDKFontInfo*)font_data;
    JNIEnv* env = jdkFontInfo->env;
    jobject font2D = jdkFontInfo->font2D;
    *glyph = (hb_codepoint_t)env->CallIntMethod(
              font2D, sunFontIDs.f2dCharToGlyphMID, unicode);
    if (env->ExceptionOccurred())
    {
        env->ExceptionClear();
    }
    if ((int)*glyph < 0) {
        *glyph = 0;
    }
    return (*glyph != 0);
}

static hb_bool_t
hb_jdk_get_variation_glyph (hb_font_t *font HB_UNUSED,
		 void *font_data,
		 hb_codepoint_t unicode,
		 hb_codepoint_t variation_selector,
		 hb_codepoint_t *glyph,
		 void *user_data HB_UNUSED)
{

    JDKFontInfo *jdkFontInfo = (JDKFontInfo*)font_data;
    JNIEnv* env = jdkFontInfo->env;
    jobject font2D = jdkFontInfo->font2D;
    *glyph = (hb_codepoint_t)env->CallIntMethod(
              font2D, sunFontIDs.f2dCharToVariationGlyphMID, 
              unicode, variation_selector);
    if (env->ExceptionOccurred())
    {
        env->ExceptionClear();
    }
    if ((int)*glyph < 0) {
        *glyph = 0;
    }
    return (*glyph != 0);
}

static hb_position_t
hb_jdk_get_glyph_h_advance (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   hb_codepoint_t glyph,
			   void *user_data HB_UNUSED)
{

    float fadv = 0.0f;
    if ((glyph & 0xfffe) == 0xfffe) {
        return 0; // JDK uses this glyph code.
    }

    JDKFontInfo *jdkFontInfo = (JDKFontInfo*)font_data;
    JNIEnv* env = jdkFontInfo->env;
    jobject fontStrike = jdkFontInfo->fontStrike;
    jobject pt = env->CallObjectMethod(fontStrike,
                                       sunFontIDs.getGlyphMetricsMID, glyph);

    if (pt == NULL) {
        return 0;
    }
    fadv = env->GetFloatField(pt, sunFontIDs.xFID);
    fadv *= jdkFontInfo->devScale;
    env->DeleteLocalRef(pt);

    return HBFloatToFixed(fadv);
}

static hb_position_t
hb_jdk_get_glyph_v_advance (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   hb_codepoint_t glyph,
			   void *user_data HB_UNUSED)
{

    float fadv = 0.0f;
    if ((glyph & 0xfffe) == 0xfffe) {
        return 0; // JDK uses this glyph code.
    }

    JDKFontInfo *jdkFontInfo = (JDKFontInfo*)font_data;
    JNIEnv* env = jdkFontInfo->env;
    jobject fontStrike = jdkFontInfo->fontStrike;
    jobject pt = env->CallObjectMethod(fontStrike,
                                       sunFontIDs.getGlyphMetricsMID, glyph);

    if (pt == NULL) {
        return 0;
    }
    fadv = env->GetFloatField(pt, sunFontIDs.yFID);
    env->DeleteLocalRef(pt);

    return HBFloatToFixed(fadv);

}

static hb_bool_t
hb_jdk_get_glyph_h_origin (hb_font_t *font HB_UNUSED,
			  void *font_data HB_UNUSED,
			  hb_codepoint_t glyph HB_UNUSED,
			  hb_position_t *x HB_UNUSED,
			  hb_position_t *y HB_UNUSED,
			  void *user_data HB_UNUSED)
{
  /* We always work in the horizontal coordinates. */
  return true;
}

static hb_bool_t
hb_jdk_get_glyph_v_origin (hb_font_t *font HB_UNUSED,
			  void *font_data,
			  hb_codepoint_t glyph,
			  hb_position_t *x,
			  hb_position_t *y,
			  void *user_data HB_UNUSED)
{
  return false;
}

static hb_position_t
hb_jdk_get_glyph_h_kerning (hb_font_t *font,
			   void *font_data,
			   hb_codepoint_t lejdk_glyph,
			   hb_codepoint_t right_glyph,
			   void *user_data HB_UNUSED)
{
  /* Not implemented. This seems to be in the HB API
   * as a way to fall back to Freetype's kerning support
   * which could be based on some on-the fly glyph analysis.
   * But more likely it reads the kern table. That is easy
   * enough code to add if we find a need to fall back
   * to that instead of using gpos. It seems like if
   * there is a gpos table at all, the practice is to
   * use that and ignore kern, no matter that gpos does
   * not implement the kern feature.
   */
  return 0;
}

static hb_position_t
hb_jdk_get_glyph_v_kerning (hb_font_t *font HB_UNUSED,
			   void *font_data HB_UNUSED,
			   hb_codepoint_t top_glyph HB_UNUSED,
			   hb_codepoint_t bottom_glyph HB_UNUSED,
			   void *user_data HB_UNUSED)
{
  /* OpenType doesn't have vertical-kerning other than GPOS. */
  return 0;
}

static hb_bool_t
hb_jdk_get_glyph_extents (hb_font_t *font HB_UNUSED,
			 void *font_data,
			 hb_codepoint_t glyph,
			 hb_glyph_extents_t *extents,
			 void *user_data HB_UNUSED)
{
  /* TODO */
  return false;
}

static hb_bool_t
hb_jdk_get_glyph_contour_point (hb_font_t *font HB_UNUSED,
			       void *font_data,
			       hb_codepoint_t glyph,
			       unsigned int point_index,
			       hb_position_t *x,
			       hb_position_t *y,
			       void *user_data HB_UNUSED)
{
    if ((glyph & 0xfffe) == 0xfffe) {
        *x = 0; *y = 0;
        return true;
    }

    JDKFontInfo *jdkFontInfo = (JDKFontInfo*)font_data;
    JNIEnv* env = jdkFontInfo->env;
    jobject fontStrike = jdkFontInfo->fontStrike;
    jobject pt = env->CallObjectMethod(fontStrike,
                                       sunFontIDs.getGlyphPointMID,
                                       glyph, point_index);

    if (pt == NULL) {
        *x = 0; *y = 0;
        return true;
    }
    *x = HBFloatToFixed(env->GetFloatField(pt, sunFontIDs.xFID));
    *y = HBFloatToFixed(env->GetFloatField(pt, sunFontIDs.yFID));
    env->DeleteLocalRef(pt);

  return true;
}

static hb_bool_t
hb_jdk_get_glyph_name (hb_font_t *font HB_UNUSED,
		      void *font_data,
		      hb_codepoint_t glyph,
		      char *name, unsigned int size,
		      void *user_data HB_UNUSED)
{
  return false;
}

static hb_bool_t
hb_jdk_get_glyph_from_name (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   const char *name, int len,
			   hb_codepoint_t *glyph,
			   void *user_data HB_UNUSED)
{
  return false;
}

// remind : can we initialise this from the code we call
// from the class static method in Java to make it
// completely thread safe.
static hb_font_funcs_t *
_hb_jdk_get_font_funcs (void)
{
  static hb_font_funcs_t *jdk_ffuncs = NULL;
  hb_font_funcs_t *ff;

  if (!jdk_ffuncs) {
      ff = hb_font_funcs_create();

      hb_font_funcs_set_nominal_glyph_func(ff, hb_jdk_get_nominal_glyph, NULL, NULL);
      hb_font_funcs_set_variation_glyph_func(ff, hb_jdk_get_variation_glyph, NULL, NULL);
      hb_font_funcs_set_glyph_h_advance_func(ff,
                    hb_jdk_get_glyph_h_advance, NULL, NULL);
      hb_font_funcs_set_glyph_v_advance_func(ff,
                    hb_jdk_get_glyph_v_advance, NULL, NULL);
      hb_font_funcs_set_glyph_h_origin_func(ff,
                    hb_jdk_get_glyph_h_origin, NULL, NULL);
      hb_font_funcs_set_glyph_v_origin_func(ff,
                    hb_jdk_get_glyph_v_origin, NULL, NULL);
      hb_font_funcs_set_glyph_h_kerning_func(ff,
                    hb_jdk_get_glyph_h_kerning, NULL, NULL);
      hb_font_funcs_set_glyph_v_kerning_func(ff,
                    hb_jdk_get_glyph_v_kerning, NULL, NULL);
      hb_font_funcs_set_glyph_extents_func(ff,
                    hb_jdk_get_glyph_extents, NULL, NULL);
      hb_font_funcs_set_glyph_contour_point_func(ff,
                    hb_jdk_get_glyph_contour_point, NULL, NULL);
      hb_font_funcs_set_glyph_name_func(ff,
                    hb_jdk_get_glyph_name, NULL, NULL);
      hb_font_funcs_set_glyph_from_name_func(ff,
                    hb_jdk_get_glyph_from_name, NULL, NULL);
      hb_font_funcs_make_immutable(ff); // done setting functions.
      jdk_ffuncs = ff;
  }
  return jdk_ffuncs;
}

static void _do_nothing(void) {
}

static void _free_nothing(void*) {
}

static hb_blob_t *
reference_table(hb_face_t *face HB_UNUSED, hb_tag_t tag, void *user_data) {

  JDKFontInfo *jdkFontInfo = (JDKFontInfo*)user_data;
  JNIEnv* env = jdkFontInfo->env;
  jobject font2D = jdkFontInfo->font2D;
  jsize length = 0;
  void* buffer = NULL;
  int cacheIdx = 0;

  // HB_TAG_NONE is 0 and is used to get the whole font file.
  // It is not expected to be needed for JDK.
  if (tag == 0 || jdkFontInfo->layoutTables == NULL) {
      return NULL;
  }

  for (cacheIdx=0; cacheIdx<LAYOUTCACHE_ENTRIES; cacheIdx++) {
    if (tag == jdkFontInfo->layoutTables->entries[cacheIdx].tag) break;
  }

  if (cacheIdx < LAYOUTCACHE_ENTRIES) { // if found
      if (jdkFontInfo->layoutTables->entries[cacheIdx].len != -1) {
          length = jdkFontInfo->layoutTables->entries[cacheIdx].len;
          buffer = (void*)jdkFontInfo->layoutTables->entries[cacheIdx].ptr;
      }
  }

  if (buffer == NULL) {
      jbyteArray tableBytes = (jbyteArray)
         env->CallObjectMethod(font2D, sunFontIDs.getTableBytesMID, tag);
      if (tableBytes == NULL) {
          return NULL;
      }
      length = env->GetArrayLength(tableBytes);
      buffer = calloc(length, sizeof(jbyte));
      if (buffer == NULL) {
          return NULL;
      }
      env->GetByteArrayRegion(tableBytes, 0, length, (jbyte*)buffer);

     if (cacheIdx >= LAYOUTCACHE_ENTRIES) { // not a cacheable table
          return hb_blob_create((const char *)buffer, length,
                                 HB_MEMORY_MODE_WRITABLE,
                                 buffer, free);
      } else {
        jdkFontInfo->layoutTables->entries[cacheIdx].len = length;
        jdkFontInfo->layoutTables->entries[cacheIdx].ptr = buffer;
      }
  }

  return hb_blob_create((const char *)buffer, length,
                         HB_MEMORY_MODE_READONLY,
                         NULL, _free_nothing);
}



hb_face_t*
hb_jdk_face_create(JDKFontInfo *jdkFontInfo,
                   hb_destroy_func_t destroy) {

    hb_face_t *face =
         hb_face_create_for_tables(reference_table, jdkFontInfo, destroy);

    return face;
}

static hb_font_t* _hb_jdk_font_create(JDKFontInfo *jdkFontInfo,
                                      hb_destroy_func_t destroy) {

    hb_font_t *font;
    hb_face_t *face;

    face = hb_jdk_face_create(jdkFontInfo, destroy);
    font = hb_font_create(face);
    hb_face_destroy (face);
    hb_font_set_funcs (font,
                       _hb_jdk_get_font_funcs (),
                       jdkFontInfo, (hb_destroy_func_t) _do_nothing);
    hb_font_set_scale (font,
                      HBFloatToFixed(jdkFontInfo->ptSize*jdkFontInfo->devScale),
                      HBFloatToFixed(jdkFontInfo->ptSize*jdkFontInfo->devScale));
  return font;
}

#ifdef MACOSX
static hb_font_t* _hb_jdk_ct_font_create(JDKFontInfo *jdkFontInfo) {

    hb_font_t *font = NULL;
    hb_face_t *face = NULL;
    if (jdkFontInfo->nativeFont == 0) {
        return NULL;
    }
    face = hb_coretext_face_create((CGFontRef)(jdkFontInfo->nativeFont));
    font = hb_font_create(face);
    hb_face_destroy(face);

    hb_font_set_scale(font,
                     HBFloatToFixed(jdkFontInfo->ptSize),
                     HBFloatToFixed(jdkFontInfo->ptSize));
    return font;
}
#endif

hb_font_t* hb_jdk_font_create(JDKFontInfo *jdkFontInfo,
                             hb_destroy_func_t destroy) {

   hb_font_t* font = NULL;

#ifdef MACOSX
     if (jdkFontInfo->aat) {
         font = _hb_jdk_ct_font_create(jdkFontInfo);
     }
#endif
    if (font == NULL) {
        font = _hb_jdk_font_create(jdkFontInfo, destroy);
    }
    return font;
}

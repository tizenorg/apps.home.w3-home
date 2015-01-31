/*
 * Samsung API
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

enum LANGUAGE {
	LANG_ENGLISH,
	LANG_KOREAN,
	LANG_JAPANESS,
	LANG_CHINESS,
	LANG_UNKNOWN
};

extern int add_viewer_ucol_init(void);

extern int add_viewer_ucol_compare(const char *src, const char *dest);
extern int add_viewer_ucol_case_compare(const char *src, const char *dest);

extern int add_viewer_ucol_ncompare(const char *src, const char *dest, int len);
extern int add_viewer_ucol_case_ncompare(const char *src, const char *dest, int len);

extern int add_viewer_ucol_search(const char *haystack, const char *needle);
extern int add_viewer_ucol_case_search(const char *haystack, const char *needle);

extern int add_viewer_ucol_compare_first_letters(const char *src, const char *letters);
extern int add_viewer_ucol_detect_lang(int ch);
extern int add_viewer_ucol_fini(void);
extern const int add_viewer_ucol_current_lang(void);
extern int add_viewer_ucol_is_alpha(const char *name);

/* End of a file */


/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * synth.h - synthetic HuCard-shaped images generated at test time (D-59). Nothing ROM-like is ever committed: the bytes exist
 * only in memory, from an integer generator, and carry no content of any game.
 */
#ifndef DS_SYNTH_H
#define DS_SYNTH_H

#include <stddef.h>

/* the generator spec string: part of every recipe, so a change of the generator is a change of recipe */
const char *synth_spec(void);

/* natural size in bytes of a recipe into *n and return 0, or return -1 when the recipe is unknown; len:0 is valid and gives 0 */
int synth_parse(const char *recipe, size_t *n);

/* natural size in bytes of a recipe, 0 when the recipe is unknown (and for len:0: use synth_parse to tell them apart) */
size_t synth_size(const char *recipe);

/*
 * Fill buf with the first n bytes of the image of a recipe; the image continues past its natural size with the same generator,
 * so an over-long request has real bytes to point at. Returns 0, or -1 when the recipe is unknown.
 */
int synth_fill(const char *recipe, unsigned char *buf, size_t n);

#endif

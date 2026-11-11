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

/*
 * The recipe name of the bytes that synth_fill(fixture, buf, len) writes, into out (cap bytes): the fixture itself when len is its
 * natural size, otherwise plain-*:LEN for a plain-* fixture and len:LEN for a len:N fixture (the pure stream). 0, or -1 when the
 * fixture is unknown or the name does not fit. scripts/conformance-run fixtures and the runner's --fixtures list use it, so the
 * list of recipes the gate emits is derived from the same resolver the cases use.
 */
int synth_name(const char *fixture, size_t len, char *out, size_t cap);

#endif

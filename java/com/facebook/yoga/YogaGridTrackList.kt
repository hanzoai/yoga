/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.yoga

/** An ordered track list for grid-template-rows/columns and grid-auto-rows/columns. */
public class YogaGridTrackList {
  private val tracks: MutableList<YogaGridTrackValue> = ArrayList()

  public val size: Int
    get() = tracks.size

  public fun addTrack(track: YogaGridTrackValue) {
    tracks.add(track)
  }

  public operator fun get(index: Int): YogaGridTrackValue = tracks[index]
}

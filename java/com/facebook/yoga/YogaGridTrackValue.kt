/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.yoga

/** A single track sizing function for grid-template-rows/columns and grid-auto-rows/columns. */
public class YogaGridTrackValue
private constructor(
    public val type: YogaGridTrackType,
    public val value: Float,
    public val min: YogaGridTrackValue?,
    public val max: YogaGridTrackValue?,
) {
  public companion object {
    @JvmStatic
    public fun auto(): YogaGridTrackValue =
        YogaGridTrackValue(YogaGridTrackType.AUTO, 0f, null, null)

    @JvmStatic
    public fun points(points: Float): YogaGridTrackValue =
        YogaGridTrackValue(YogaGridTrackType.POINTS, points, null, null)

    @JvmStatic
    public fun percent(percent: Float): YogaGridTrackValue =
        YogaGridTrackValue(YogaGridTrackType.PERCENT, percent, null, null)

    @JvmStatic
    public fun fr(fr: Float): YogaGridTrackValue =
        YogaGridTrackValue(YogaGridTrackType.FR, fr, null, null)

    @JvmStatic
    public fun minMax(min: YogaGridTrackValue, max: YogaGridTrackValue): YogaGridTrackValue =
        YogaGridTrackValue(YogaGridTrackType.MINMAX, 0f, min, max)
  }
}

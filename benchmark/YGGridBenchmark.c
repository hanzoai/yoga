/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <yoga/Yoga.h>

#define NUM_REPETITIONS 1000

#define YGBENCHMARKS(BLOCK)                \
  int main(int argc, char const* argv[]) { \
    (void)argc;                            \
    (void)argv;                            \
    clock_t __start;                       \
    clock_t __endTimes[NUM_REPETITIONS];   \
    {                                      \
      BLOCK                                \
    }                                      \
    return 0;                              \
  }

#define YGBENCHMARK(NAME, BLOCK)                         \
  __start = clock();                                     \
  for (uint32_t __i = 0; __i < NUM_REPETITIONS; __i++) { \
    {BLOCK} __endTimes[__i] = clock();                   \
  }                                                      \
  __printBenchmarkResult(NAME, __start, __endTimes);

static int __compareDoubles(const void* a, const void* b) {
  double arg1 = *(const double*)a;
  double arg2 = *(const double*)b;

  if (arg1 < arg2) {
    return -1;
  }

  if (arg1 > arg2) {
    return 1;
  }

  return 0;
}

static void
__printBenchmarkResult(char* name, clock_t start, const clock_t* endTimes) {
  double timesInMs[NUM_REPETITIONS];
  double mean = 0;
  clock_t lastEnd = start;
  for (uint32_t i = 0; i < NUM_REPETITIONS; i++) {
    timesInMs[i] =
        ((double)(endTimes[i] - lastEnd)) / (double)CLOCKS_PER_SEC * 1000;
    lastEnd = endTimes[i];
    mean += timesInMs[i];
  }
  mean /= NUM_REPETITIONS;

  qsort(timesInMs, NUM_REPETITIONS, sizeof(double), __compareDoubles);
  double median = timesInMs[NUM_REPETITIONS / 2];

  double variance = 0;
  for (uint32_t i = 0; i < NUM_REPETITIONS; i++) {
    variance += pow(timesInMs[i] - mean, 2);
  }
  variance /= NUM_REPETITIONS;
  double stddev = sqrt(variance);

  printf("%s: median: %lf ms, stddev: %lf ms\n", name, median, stddev);
}

static YGSize _measure(
    YGNodeConstRef node,
    float width,
    YGMeasureMode widthMode,
    float height,
    YGMeasureMode heightMode) {
  (void)node;
  return (YGSize){
      .width = widthMode == YGMeasureModeUndefined ? 10 : width,
      .height = heightMode == YGMeasureModeUndefined ? 10 : height,
  };
}

static YGSize _measureFixed(
    YGNodeConstRef node,
    float width,
    YGMeasureMode widthMode,
    float height,
    YGMeasureMode heightMode) {
  (void)node;
  (void)width;
  (void)widthMode;
  (void)height;
  (void)heightMode;
  return (YGSize){
      .width = 50,
      .height = 50,
  };
}

// A track sizing function. Every track carries a type and a value, except
// minmax() which carries a bound on each side.
typedef struct {
  YGGridTrackType type;
  float value;
  YGGridTrackType minType;
  float minValue;
  YGGridTrackType maxType;
  float maxValue;
} Track;

#define POINTS(v) {.type = YGGridTrackTypePoints, .value = (v)}
#define PERCENT(v) {.type = YGGridTrackTypePercent, .value = (v)}
#define FR(v) {.type = YGGridTrackTypeFr, .value = (v)}
#define AUTO {.type = YGGridTrackTypeAuto}
#define MINMAX(nt, nv, xt, xv)                                        \
  {.type = YGGridTrackTypeMinmax, .minType = (nt), .minValue = (nv), \
   .maxType = (xt), .maxValue = (xv)}

// The three setters that write one axis of a grid's template.
typedef struct {
  void (*setCount)(YGNodeRef, size_t);
  void (*setTrack)(YGNodeRef, size_t, YGGridTrackType, float);
  void (*setMinMax)(
      YGNodeRef,
      size_t,
      YGGridTrackType,
      float,
      YGGridTrackType,
      float);
} Axis;

static const Axis kColumns = {
    .setCount = YGNodeStyleSetGridTemplateColumnsCount,
    .setTrack = YGNodeStyleSetGridTemplateColumn,
    .setMinMax = YGNodeStyleSetGridTemplateColumnMinMax,
};

static const Axis kRows = {
    .setCount = YGNodeStyleSetGridTemplateRowsCount,
    .setTrack = YGNodeStyleSetGridTemplateRow,
    .setMinMax = YGNodeStyleSetGridTemplateRowMinMax,
};

static void applyTracks(
    YGNodeRef node,
    const Axis* axis,
    const Track* tracks,
    size_t count) {
  axis->setCount(node, count);
  for (size_t i = 0; i < count; i++) {
    const Track* track = &tracks[i];
    if (track->type == YGGridTrackTypeMinmax) {
      axis->setMinMax(
          node,
          i,
          track->minType,
          track->minValue,
          track->maxType,
          track->maxValue);
    } else {
      axis->setTrack(node, i, track->type, track->value);
    }
  }
}

#define APPLY(node, axis, tracks) \
  applyTracks((node), (axis), (tracks), sizeof(tracks) / sizeof(*(tracks)))

static const Track kFixed3x100[] = {POINTS(100), POINTS(100), POINTS(100)};
static const Track kFixed3x80[] = {POINTS(80), POINTS(80), POINTS(80)};
static const Track kAuto2[] = {AUTO, AUTO};
static const Track kAuto3[] = {AUTO, AUTO, AUTO};
static const Track kAuto4[] = {AUTO, AUTO, AUTO, AUTO};
static const Track kFr2[] = {FR(1), FR(1)};
static const Track kFr3[] = {FR(1), FR(1), FR(1)};
static const Track kFr4[] = {FR(1), FR(1), FR(1), FR(1)};
static const Track kFr5[] = {FR(1), FR(1), FR(1), FR(1), FR(1)};
static const Track kPercent3Columns[] = {PERCENT(25), PERCENT(50), PERCENT(25)};
static const Track kPercent3Rows[] = {
    PERCENT(33.33f),
    PERCENT(33.33f),
    PERCENT(33.33f)};
static const Track kMixedColumns[] = {POINTS(200), FR(1), POINTS(200)};
static const Track kMixedRows[] = {POINTS(60), FR(1), POINTS(40)};

#define MINMAX_POINTS_FR(n, x) \
  MINMAX(YGGridTrackTypePoints, (n), YGGridTrackTypeFr, (x))
#define MINMAX_POINTS_AUTO(n) \
  MINMAX(YGGridTrackTypePoints, (n), YGGridTrackTypeAuto, 0)

static const Track kMinmaxColumns[] = {
    MINMAX_POINTS_FR(100, 1),
    MINMAX_POINTS_FR(100, 1),
    MINMAX_POINTS_FR(100, 1)};
static const Track kMinmaxRows[] = {
    MINMAX_POINTS_AUTO(50),
    MINMAX_POINTS_AUTO(50),
    MINMAX_POINTS_AUTO(50)};

// 5 repeats of a 4-track group, and 10 repeats of a 5-track group.
#define MIXED_COLUMN_GROUP POINTS(100), FR(1), AUTO, MINMAX_POINTS_FR(50, 1)
#define MIXED_ROW_GROUP \
  POINTS(40), FR(1), AUTO, MINMAX_POINTS_AUTO(30), FR(2)

static const Track kMixed20Columns[] = {
    MIXED_COLUMN_GROUP,
    MIXED_COLUMN_GROUP,
    MIXED_COLUMN_GROUP,
    MIXED_COLUMN_GROUP,
    MIXED_COLUMN_GROUP};

static const Track kMixed50Rows[] = {
    MIXED_ROW_GROUP,
    MIXED_ROW_GROUP,
    MIXED_ROW_GROUP,
    MIXED_ROW_GROUP,
    MIXED_ROW_GROUP,
    MIXED_ROW_GROUP,
    MIXED_ROW_GROUP,
    MIXED_ROW_GROUP,
    MIXED_ROW_GROUP,
    MIXED_ROW_GROUP};

YGBENCHMARKS({
  // Scenario 1: Basic fixed-size grid
  YGBENCHMARK("Grid 3x3 fixed tracks", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    YGNodeStyleSetWidth(root, 300);
    YGNodeStyleSetHeight(root, 300);
    APPLY(root, &kColumns, kFixed3x100);
    APPLY(root, &kRows, kFixed3x100);

    for (uint32_t i = 0; i < 9; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeInsertChild(root, child, i);
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 2: Grid with auto-sized items
  YGBENCHMARK("Grid 3x3 auto tracks", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    APPLY(root, &kColumns, kAuto3);
    APPLY(root, &kRows, kAuto3);

    for (uint32_t i = 0; i < 9; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeSetMeasureFunc(child, _measureFixed);
      YGNodeInsertChild(root, child, i);
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 3: Grid with fr units
  YGBENCHMARK("Grid 3x3 fr tracks", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    YGNodeStyleSetWidth(root, 300);
    YGNodeStyleSetHeight(root, 300);
    APPLY(root, &kColumns, kFr3);
    APPLY(root, &kRows, kFr3);

    for (uint32_t i = 0; i < 9; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeInsertChild(root, child, i);
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 4: Grid with gaps
  YGBENCHMARK("Grid 4x4 with gaps", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    YGNodeStyleSetWidth(root, 400);
    YGNodeStyleSetHeight(root, 400);
    YGNodeStyleSetGap(root, YGGutterAll, 10);
    APPLY(root, &kColumns, kFr4);
    APPLY(root, &kRows, kFr4);

    for (uint32_t i = 0; i < 16; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeInsertChild(root, child, i);
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 5: Mixed fixed and flexible tracks
  YGBENCHMARK("Grid mixed tracks (fixed + fr)", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    YGNodeStyleSetWidth(root, 800);
    YGNodeStyleSetHeight(root, 600);
    APPLY(root, &kColumns, kMixedColumns);
    APPLY(root, &kRows, kMixedRows);

    for (uint32_t i = 0; i < 9; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeInsertChild(root, child, i);
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 6: Grid with spanning items
  YGBENCHMARK("Grid with spanning items", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    YGNodeStyleSetWidth(root, 400);
    YGNodeStyleSetHeight(root, 400);
    YGNodeStyleSetGap(root, YGGutterAll, 8);
    APPLY(root, &kColumns, kFr4);
    APPLY(root, &kRows, kFr4);

    YGNodeRef child1 = YGNodeNew();
    YGNodeStyleSetGridColumnStart(child1, 1);
    YGNodeStyleSetGridColumnEndSpan(child1, 2);
    YGNodeInsertChild(root, child1, 0);

    YGNodeRef child2 = YGNodeNew();
    YGNodeStyleSetGridRowStart(child2, 1);
    YGNodeStyleSetGridRowEndSpan(child2, 2);
    YGNodeInsertChild(root, child2, 1);

    YGNodeRef child3 = YGNodeNew();
    YGNodeStyleSetGridColumnStart(child3, 3);
    YGNodeStyleSetGridColumnEndSpan(child3, 2);
    YGNodeStyleSetGridRowStart(child3, 3);
    YGNodeStyleSetGridRowEndSpan(child3, 2);
    YGNodeInsertChild(root, child3, 2);

    for (uint32_t i = 0; i < 8; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeInsertChild(root, child, 3 + i);
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 7: Auto-placement
  YGBENCHMARK("Grid auto-placement 5x5", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    YGNodeStyleSetWidth(root, 500);
    YGNodeStyleSetHeight(root, 500);
    APPLY(root, &kColumns, kFr5);
    APPLY(root, &kRows, kFr5);

    for (uint32_t i = 0; i < 25; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeInsertChild(root, child, i);
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 8: Nested grids
  YGBENCHMARK("Nested grids 3x3 with 2x2 children", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    YGNodeStyleSetWidth(root, 600);
    YGNodeStyleSetHeight(root, 600);
    YGNodeStyleSetGap(root, YGGutterAll, 10);
    APPLY(root, &kColumns, kFr3);
    APPLY(root, &kRows, kFr3);

    for (uint32_t i = 0; i < 9; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeStyleSetDisplay(child, YGDisplayGrid);
      YGNodeStyleSetGap(child, YGGutterAll, 4);
      APPLY(child, &kColumns, kFr2);
      APPLY(child, &kRows, kFr2);
      YGNodeInsertChild(root, child, i);

      for (uint32_t j = 0; j < 4; j++) {
        YGNodeRef grandChild = YGNodeNew();
        YGNodeInsertChild(child, grandChild, j);
      }
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 9: Grid with alignment
  YGBENCHMARK("Grid with alignment", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    YGNodeStyleSetWidth(root, 400);
    YGNodeStyleSetHeight(root, 400);
    YGNodeStyleSetJustifyContent(root, YGJustifyCenter);
    YGNodeStyleSetAlignContent(root, YGAlignCenter);
    YGNodeStyleSetGap(root, YGGutterAll, 10);
    APPLY(root, &kColumns, kFixed3x80);
    APPLY(root, &kRows, kFixed3x80);

    for (uint32_t i = 0; i < 9; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeStyleSetAlignSelf(child, YGAlignCenter);
      YGNodeStyleSetWidth(child, 60);
      YGNodeStyleSetHeight(child, 60);
      YGNodeInsertChild(root, child, i);
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 10: Grid with intrinsic sizing and measure functions
  YGBENCHMARK("Grid auto tracks with measure", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    YGNodeStyleSetWidth(root, 400);
    APPLY(root, &kColumns, kAuto4);
    APPLY(root, &kRows, kAuto4);

    for (uint32_t i = 0; i < 16; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeSetMeasureFunc(child, _measure);
      YGNodeInsertChild(root, child, i);
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 11: minmax tracks
  YGBENCHMARK("Grid minmax tracks", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    YGNodeStyleSetWidth(root, 600);
    YGNodeStyleSetHeight(root, 400);
    YGNodeStyleSetGap(root, YGGutterAll, 10);
    APPLY(root, &kColumns, kMinmaxColumns);
    APPLY(root, &kRows, kMinmaxRows);

    for (uint32_t i = 0; i < 9; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeSetMeasureFunc(child, _measureFixed);
      YGNodeInsertChild(root, child, i);
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 12: Indefinite container size
  YGBENCHMARK("Grid indefinite container", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    APPLY(root, &kColumns, kAuto3);
    APPLY(root, &kRows, kAuto2);

    for (uint32_t i = 0; i < 6; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeStyleSetWidth(child, 80);
      YGNodeStyleSetHeight(child, 60);
      YGNodeInsertChild(root, child, i);
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 13: Grid with percentage tracks
  YGBENCHMARK("Grid percentage tracks", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    YGNodeStyleSetWidth(root, 400);
    YGNodeStyleSetHeight(root, 300);
    APPLY(root, &kColumns, kPercent3Columns);
    APPLY(root, &kRows, kPercent3Rows);

    for (uint32_t i = 0; i < 9; i++) {
      YGNodeRef child = YGNodeNew();
      YGNodeInsertChild(root, child, i);
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });

  // Scenario 14: Stress test - 1000 items with mixed tracks and spanning
  YGBENCHMARK("Stress test 1000 items mixed", {
    YGNodeRef root = YGNodeNew();
    YGNodeStyleSetDisplay(root, YGDisplayGrid);
    YGNodeStyleSetWidth(root, 2000);
    YGNodeStyleSetHeight(root, 5000);
    YGNodeStyleSetGap(root, YGGutterColumn, 8);
    YGNodeStyleSetGap(root, YGGutterRow, 4);
    APPLY(root, &kColumns, kMixed20Columns);
    APPLY(root, &kRows, kMixed50Rows);

    uint32_t childIndex = 0;

    for (uint32_t row = 1; row <= 50; row++) {
      for (uint32_t col = 1; col <= 20; col++) {
        YGNodeRef child = YGNodeNew();

        if (childIndex % 30 == 0 && col <= 17) {
          YGNodeStyleSetGridColumnStart(child, (int)col);
          YGNodeStyleSetGridColumnEndSpan(child, 3);
          YGNodeStyleSetGridRowStart(child, (int)row);
        } else if (childIndex % 40 == 0 && col <= 16) {
          YGNodeStyleSetGridColumnStart(child, (int)col);
          YGNodeStyleSetGridColumnEndSpan(child, 4);
          YGNodeStyleSetGridRowStart(child, (int)row);
        } else if (childIndex % 50 == 0 && row <= 48) {
          YGNodeStyleSetGridColumnStart(child, (int)col);
          YGNodeStyleSetGridRowStart(child, (int)row);
          YGNodeStyleSetGridRowEndSpan(child, 2);
        } else if (childIndex % 70 == 0 && row <= 47) {
          YGNodeStyleSetGridColumnStart(child, (int)col);
          YGNodeStyleSetGridRowStart(child, (int)row);
          YGNodeStyleSetGridRowEndSpan(child, 3);
        } else if (childIndex % 100 == 0 && col <= 18 && row <= 48) {
          YGNodeStyleSetGridColumnStart(child, (int)col);
          YGNodeStyleSetGridColumnEndSpan(child, 2);
          YGNodeStyleSetGridRowStart(child, (int)row);
          YGNodeStyleSetGridRowEndSpan(child, 2);
        } else if (childIndex % 150 == 0 && col <= 17 && row <= 48) {
          YGNodeStyleSetGridColumnStart(child, (int)col);
          YGNodeStyleSetGridColumnEndSpan(child, 3);
          YGNodeStyleSetGridRowStart(child, (int)row);
          YGNodeStyleSetGridRowEndSpan(child, 2);
        } else if (childIndex % 200 == 0 && col <= 18 && row <= 47) {
          YGNodeStyleSetGridColumnStart(child, (int)col);
          YGNodeStyleSetGridColumnEndSpan(child, 2);
          YGNodeStyleSetGridRowStart(child, (int)row);
          YGNodeStyleSetGridRowEndSpan(child, 3);
        } else if (childIndex % 300 == 0 && col <= 16 && row <= 47) {
          YGNodeStyleSetGridColumnStart(child, (int)col);
          YGNodeStyleSetGridColumnEndSpan(child, 4);
          YGNodeStyleSetGridRowStart(child, (int)row);
          YGNodeStyleSetGridRowEndSpan(child, 3);
        } else if (childIndex % 500 == 0 && col <= 15 && row <= 46) {
          YGNodeStyleSetGridColumnStart(child, (int)col);
          YGNodeStyleSetGridColumnEndSpan(child, 5);
          YGNodeStyleSetGridRowStart(child, (int)row);
          YGNodeStyleSetGridRowEndSpan(child, 4);
        }

        YGNodeInsertChild(root, child, childIndex);
        childIndex++;

        if (childIndex >= 1000)
          break;
      }
      if (childIndex >= 1000)
        break;
    }

    YGNodeCalculateLayout(root, YGUndefined, YGUndefined, YGDirectionLTR);
    YGNodeFreeRecursive(root);
  });
});

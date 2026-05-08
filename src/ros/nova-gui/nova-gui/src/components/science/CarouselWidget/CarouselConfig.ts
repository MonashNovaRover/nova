// ============================================================================
// CONFIGURATION - Edit these values to customize the carousel appearance
// ============================================================================

// All colors used in the carousel
export enum Color {
  // Cuvette fills (70% opacity)
  BLUE = '#4da3ff70',
  PURPLE = '#AD2D6770',
  PINK = '#F770AD70',
  RED = '#EE000070',
  YELLOW = '#FFC90270',

  // UI colors
  EMPTY = '#1a1a1a',
  HOVER = '#3a3a3a',
  STROKE = '#ffffff90',
  TEXT = '#ffffff',
  INDICATOR = '#F770AD',

  // Group/dot colors
  ORANGE = '#FFB86C',
  GRAY = '#AAAAAA',
}

// Outer wheel: 24 cuvettes
export const OUTER_CUVETTE_COLORS = [
  Color.PURPLE, // 1
  Color.PURPLE, // 2
  Color.PURPLE, // 3
  Color.BLUE,   // 4
  Color.BLUE,   // 5
  Color.BLUE,   // 6
  Color.YELLOW, // 7
  Color.PURPLE, // 8
  Color.PURPLE, // 9
  Color.PURPLE, // 10
  Color.BLUE,   // 11
  Color.BLUE,   // 12
  Color.BLUE,   // 13
  Color.YELLOW, // 14
  Color.PURPLE, // 15
  Color.PURPLE, // 16
  Color.BLUE,   // 17
  Color.BLUE,   // 18
  Color.PINK,   // 19
  Color.PINK,   // 20
  Color.YELLOW, // 21
  Color.EMPTY,  // 22
  Color.EMPTY,  // 23
  Color.EMPTY,  // 24
];

// Inner wheel: 15 cuvettes
export const INNER_CUVETTE_COLORS = [
  Color.RED,    // 1
  Color.RED,    // 2
  Color.RED,    // 3
  Color.PINK,   // 4
  Color.PINK,   // 5
  Color.PINK,   // 6
  Color.RED,    // 7
  Color.RED,    // 8
  Color.RED,    // 9
  Color.RED,    // 10
  Color.RED,    // 11
  Color.EMPTY,  // 12
  Color.PINK,   // 13
  Color.PINK,   // 14
  Color.PINK,   // 15
];

// General colors (for convenience, references the enum)
export const COLORS = {
  hover: Color.HOVER,
  stroke: Color.STROKE,
  text: Color.TEXT,
  indicator: Color.INDICATOR,
  center: Color.EMPTY,
};

// Group border configuration
export const GROUP_BORDER = {
  color: Color.TEXT,
  width: 2,
};

// Types for groups
export interface CuvetteGroup {
  start: number;
  end: number;      // exclusive (last cuvette in group, not included in border)
  color: Color;
}

// Outer wheel groups: { start, end } are 0-indexed, end is exclusive (last cuvette not included in border)
export const OUTER_GROUPS: CuvetteGroup[] = [
  { start: 0,  end: 7,  color: Color.ORANGE },
  { start: 7,  end: 14, color: Color.GRAY },
];

// Inner wheel groups: { start, end } are 0-indexed, end is exclusive
export const INNER_GROUPS: CuvetteGroup[] = [
  { start: 3,  end: 9,  color: Color.GRAY },
  { start: 12, end: 3,  color: Color.ORANGE },
];

// Indicator dot type
export interface IndicatorDot {
  cuvette: number;       // Angular position of the dot (1-indexed cuvette number)
  color: Color;
  radius: number;
  targetCuvette: number; // Cuvette to rotate to when clicked (1-indexed)
}

// Static indicator dots (outside the carousel, don't rotate)
// Positions are cuvette numbers (1-indexed) as if 24 is at the top
// targetCuvette specifies which cuvette to rotate to when clicked
export const OUTER_INDICATOR_DOTS: IndicatorDot[] = [
  { cuvette: 6,  color: Color.ORANGE, radius: 4, targetCuvette: 19 },
  { cuvette: 13, color: Color.GRAY,   radius: 4, targetCuvette: 19 },
];
export const OUTER_INDICATOR_DOT_DISTANCE = 152;

// Inner indicator dots (inside center circle, don't rotate)
// Positions are cuvette numbers (1-indexed) for 15-segment inner wheel
// targetCuvette specifies which cuvette to rotate to when clicked
export const INNER_INDICATOR_DOTS: IndicatorDot[] = [
  { cuvette: 9, color: Color.GRAY,   radius: 4, targetCuvette: 10 },
  { cuvette: 3, color: Color.ORANGE, radius: 4, targetCuvette: 10 },
];
export const INNER_INDICATOR_DOT_DISTANCE = 37;

export const OUTER_CUVETTE_NAMES: string[] = [
  "O1 - Molish S1 1",
  "O2 - Molish S1 2",
  "O3 - Molish S1 3",
  "O4 - UV Fluor S1 1",
  "O5 - UV Fluor S1 2",
  "O6 - UV Fluor S1 3",
  "O7 - Blank S1",
  "O8 - Molish S2 1",
  "O9 - Molish S2 2",
  "O10 - Molish S2 3",
  "O11 - UV Fluor S2 1",
  "O12 - UV Fluor S2 2",
  "O13 - UV Fluor S2 3",
  "O14 - Blank S2",
  "O15 - Molish neg 1",
  "O16 - Molish pos 2",
  "O17 - UV Fluor neg 1",
  "O18 - UV Fluor pos 2",
  "O19 - Ninhydrin neg 1",
  "O20 - Ninhydrin pos 2",
  "O21 - Blank",
  "",
  "",
  "",
];

export const INNER_CUVETTE_NAMES: string[] = [
  "I1 - Nile Red S1 1",
  "I2 - Nile Red S1 2",
  "I3 - Nile Red S1 3",
  "I4 - Ninhydrin S2 1",
  "I5 - Ninhydrin S2 2",
  "I6 - Ninhydrin S2 3",
  "I7 - Nile Red S2 1",
  "I8 - Nile Red S2 2",
  "I9 - Nile Red S2 3",
  "I10 - Nile Red neg 1",
  "I11 - Nile Red pos 2",
  "",
  "I13 - Ninhydrin S1 1",
  "I14 - Ninhydrin S1 2",
  "I15 - Ninhydrin S1 3",
];

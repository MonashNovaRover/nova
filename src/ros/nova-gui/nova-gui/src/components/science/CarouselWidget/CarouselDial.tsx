import React, { useState } from "react";
import { ChevronUp, Search } from "react-feather";

// ============================================================================
// CONFIGURATION - Edit these values to customize the carousel appearance
// ============================================================================

// Cuvette color constants (70% opacity)
const CUVETTE = {
  BLUE:    '#4da3ff70',
  PURPLE:  '#AD2D6770',
  PINK:    '#F770AD70',
  RED:     '#EE000070',
  YELLOW:  '#FFC90270',
  EMPTY:   '#1a1a1a',
};

// Outer wheel: 24 cuvettes
const OUTER_CUVETTE_COLORS = [
  CUVETTE.PURPLE, // 1
  CUVETTE.PURPLE, // 2
  CUVETTE.PURPLE, // 3
  CUVETTE.BLUE,   // 4
  CUVETTE.BLUE,   // 5
  CUVETTE.BLUE,   // 6
  CUVETTE.YELLOW, // 7
  CUVETTE.PURPLE, // 8
  CUVETTE.PURPLE, // 9
  CUVETTE.PURPLE, // 10
  CUVETTE.BLUE,   // 11
  CUVETTE.BLUE,   // 12
  CUVETTE.BLUE,   // 13
  CUVETTE.YELLOW, // 14
  CUVETTE.PURPLE, // 15
  CUVETTE.PURPLE, // 16
  CUVETTE.BLUE,   // 17
  CUVETTE.BLUE,   // 18
  CUVETTE.PINK,   // 19
  CUVETTE.PINK,   // 20
  CUVETTE.YELLOW, // 21
  CUVETTE.YELLOW, // 22
  CUVETTE.YELLOW, // 23
  CUVETTE.EMPTY,  // 24
];

// Inner wheel: 15 cuvettes
const INNER_CUVETTE_COLORS = [
  CUVETTE.RED,    // 1
  CUVETTE.RED,    // 2
  CUVETTE.RED,    // 3
  CUVETTE.PINK,   // 4
  CUVETTE.PINK,   // 5
  CUVETTE.PINK,   // 6
  CUVETTE.RED,    // 7
  CUVETTE.RED,    // 8
  CUVETTE.RED,    // 9
  CUVETTE.RED,    // 10
  CUVETTE.RED,    // 11
  CUVETTE.EMPTY,  // 12
  CUVETTE.PINK,   // 13
  CUVETTE.PINK,   // 14
  CUVETTE.PINK,   // 15
];

// General colors
const COLORS = {
  hover: '#3a3a3a',        // Segment fill on hover
  stroke: '#ffffff90',       // Divider lines between segments
  text: '#ffffff',         // Segment number text
  indicator: '#F770AD',    // Chevron and search icons
  center: '#1a1a1a',       // Center circle fill
};

// ============================================================================
// END CONFIGURATION
// ============================================================================

// Types
export type SegmentState = 'empty' | 'tested' | 'error' | 'default';

export interface WheelConfig {
  current: number;
  onClick: (index: number) => void;
  colors?: string[];
  states?: SegmentState[];
}

export interface CarouselDialProps {
  outer?: WheelConfig;
  inner?: WheelConfig;
  /** @deprecated Use outer.current instead */
  cuvette?: number;
}

// Constants
const OUTER_SEGMENTS = 24;
const INNER_SEGMENTS = 15;

const OUTER_OUTER_RADIUS = 145;
const OUTER_INNER_RADIUS = 95;
const INNER_OUTER_RADIUS = 90;
const INNER_INNER_RADIUS = 45;

const OUTER_STEP = 360 / OUTER_SEGMENTS;
const INNER_STEP = 360 / INNER_SEGMENTS;

const OUTER_OFFSET = 180 - (OUTER_STEP / 2);
const INNER_OFFSET = 180 - (INNER_STEP / 2);

const CENTER = 150;

// Helper functions
function createWedgePath(
  index: number,
  totalSegments: number,
  outerRadius: number,
  innerRadius: number
): string {
  const anglePerSegment = (2 * Math.PI) / totalSegments;
  const startAngle = index * anglePerSegment - Math.PI / 2;
  const endAngle = startAngle + anglePerSegment;

  const outerStart = {
    x: CENTER + outerRadius * Math.cos(startAngle),
    y: CENTER + outerRadius * Math.sin(startAngle)
  };
  const outerEnd = {
    x: CENTER + outerRadius * Math.cos(endAngle),
    y: CENTER + outerRadius * Math.sin(endAngle)
  };
  const innerStart = {
    x: CENTER + innerRadius * Math.cos(startAngle),
    y: CENTER + innerRadius * Math.sin(startAngle)
  };
  const innerEnd = {
    x: CENTER + innerRadius * Math.cos(endAngle),
    y: CENTER + innerRadius * Math.sin(endAngle)
  };

  return `
    M ${outerStart.x} ${outerStart.y}
    A ${outerRadius} ${outerRadius} 0 0 1 ${outerEnd.x} ${outerEnd.y}
    L ${innerEnd.x} ${innerEnd.y}
    A ${innerRadius} ${innerRadius} 0 0 0 ${innerStart.x} ${innerStart.y}
    Z
  `;
}

function getTextPosition(index: number, totalSegments: number, radius: number) {
  const anglePerSegment = (2 * Math.PI) / totalSegments;
  const midAngle = index * anglePerSegment + anglePerSegment / 2 - Math.PI / 2;

  return {
    x: CENTER + radius * Math.cos(midAngle),
    y: CENTER + radius * Math.sin(midAngle),
    rotation: (midAngle * 180 / Math.PI) + 90,
  };
}

function getSegmentColor(
  index: number,
  wheelId: 'inner' | 'outer',
  isHovered: boolean
): string {
  if (isHovered) return COLORS.hover;

  const colorArray = wheelId === 'outer' ? OUTER_CUVETTE_COLORS : INNER_CUVETTE_COLORS;
  return colorArray[index] ?? COLORS.hover;
}

const CarouselDial: React.FC<CarouselDialProps> = ({
  outer,
  inner,
  cuvette,
}) => {
  const [hoveredSegment, setHoveredSegment] = useState<{ wheel: 'inner' | 'outer'; index: number } | null>(null);

  // Backward compatibility: use cuvette prop if outer/inner not provided
  const noopClick = (val:number) => {console.log(val)};
  const outerConfig: WheelConfig = outer ?? { current: cuvette ?? 0, onClick: noopClick };
  const innerConfig: WheelConfig = inner ?? { current: cuvette ?? 0, onClick: noopClick };

  const outerRotation = outerConfig.current * OUTER_STEP + OUTER_OFFSET;
  const innerRotation = innerConfig.current * INNER_STEP + INNER_OFFSET;

  const renderSegments = (
    count: number,
    outerR: number,
    innerR: number,
    config: WheelConfig,
    wheelId: 'inner' | 'outer'
  ) => {
    const textRadius = (outerR + innerR) / 2;
    const fontSize = wheelId === 'outer' ? 16 : 14;

    return Array.from({ length: count }).map((_, i) => {
      const isHovered = hoveredSegment?.wheel === wheelId && hoveredSegment?.index === i;
      const fillColor = getSegmentColor(i, wheelId, isHovered);

      const path = createWedgePath(i, count, outerR, innerR);
      const textPos = getTextPosition(i, count, textRadius);

      return (
        <g
          key={i}
          onClick={() => config.onClick(i)}
          onMouseEnter={() => setHoveredSegment({ wheel: wheelId, index: i })}
          onMouseLeave={() => setHoveredSegment(null)}
          className="cursor-pointer"
        >
          <path
            d={path}
            fill={fillColor}
            stroke={COLORS.stroke}
            strokeWidth="1"
            className="transition-colors duration-150"
          />
          <text
            x={textPos.x}
            y={textPos.y}
            fill={COLORS.text}
            fontSize={fontSize}
            textAnchor="middle"
            dominantBaseline="middle"
            transform={`rotate(${textPos.rotation}, ${textPos.x}, ${textPos.y})`}
            className="pointer-events-none select-none"
            style={{ fontWeight: 500 }}
          >
            {i + 1}
          </text>
        </g>
      );
    });
  };

  return (
    <div className="flex flex-col items-center w-full h-full">
      <Search color={COLORS.indicator} className="w-16 h-8 flex-shrink-0" />
      <div className="flex flex-row items-center w-full">
        <svg
          viewBox="0 0 300 300"
          className="w-full h-auto aspect-square"
        >
          {/* Outer wheel */}
          <g
            style={{
              transform: `rotate(${outerRotation}deg)`,
              transformOrigin: 'center',
              transition: 'transform 0.3s ease-in-out',
            }}
          >
            {renderSegments(OUTER_SEGMENTS, OUTER_OUTER_RADIUS, OUTER_INNER_RADIUS, outerConfig, 'outer')}
          </g>

          {/* Inner wheel */}
          <g
            style={{
              transform: `rotate(${innerRotation}deg)`,
              transformOrigin: 'center',
              transition: 'transform 0.3s ease-in-out',
            }}
          >
            {renderSegments(INNER_SEGMENTS, INNER_OUTER_RADIUS, INNER_INNER_RADIUS, innerConfig, 'inner')}
          </g>

          {/* Center circle */}
          <circle cx={CENTER} cy={CENTER} r="40" fill={COLORS.center} />
        </svg>
      </div>
      <ChevronUp color={COLORS.indicator} className="w-16 h-8 flex-shrink-0" />
    </div>
  );
};

export default CarouselDial;

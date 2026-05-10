import React, { useState, useRef, useEffect } from "react";
import { ChevronUp, Search } from "react-feather";
import {
  COLORS,
  GROUP_BORDER,
  OUTER_CUVETTE_COLORS,
  INNER_CUVETTE_COLORS,
  OUTER_GROUPS,
  INNER_GROUPS,
  OUTER_INDICATOR_DOTS,
  OUTER_INDICATOR_DOT_DISTANCE,
  INNER_INDICATOR_DOTS,
  INNER_INDICATOR_DOT_DISTANCE,
} from "./CarouselConfig";

export interface WheelConfig {
  current: number;
  onClick: (index: number) => void;
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

// Offsets to center the current cuvette at the top of the diagram
const OUTER_OFFSET = -OUTER_STEP / 2;
const INNER_OFFSET = -INNER_STEP / 2;

const CENTER = 156;

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

// Create arc path for a group border (inset from outer edge)
function createGroupArcPath(
  startIndex: number,
  endIndex: number,
  totalSegments: number,
  outerRadius: number,
  innerRadius: number
): string {
  // Inset the border so it sits inside the white stroke (matches stroke width)
  const inset = 1.5;
  const insetOuter = outerRadius - inset;
  const insetInner = innerRadius + inset;

  const anglePerSegment = (2 * Math.PI) / totalSegments;
  // Also inset the angles slightly so the border doesn't overlap the radial lines
  // Calculate angle inset based on 1px at the outer radius
  const angleInset = inset / outerRadius;
  const startAngle = startIndex * anglePerSegment - Math.PI / 2 + angleInset;
  const endAngle = endIndex * anglePerSegment - Math.PI / 2 - angleInset;

  const outerStart = {
    x: CENTER + insetOuter * Math.cos(startAngle),
    y: CENTER + insetOuter * Math.sin(startAngle)
  };
  const outerEnd = {
    x: CENTER + insetOuter * Math.cos(endAngle),
    y: CENTER + insetOuter * Math.sin(endAngle)
  };
  const innerStart = {
    x: CENTER + insetInner * Math.cos(startAngle),
    y: CENTER + insetInner * Math.sin(startAngle)
  };
  const innerEnd = {
    x: CENTER + insetInner * Math.cos(endAngle),
    y: CENTER + insetInner * Math.sin(endAngle)
  };

  // Determine if we need the large arc flag (more than 180 degrees)
  const arcSpan = endIndex - startIndex;
  const largeArc = arcSpan > totalSegments / 2 ? 1 : 0;

  return `
    M ${outerStart.x} ${outerStart.y}
    A ${insetOuter} ${insetOuter} 0 ${largeArc} 1 ${outerEnd.x} ${outerEnd.y}
    L ${innerEnd.x} ${innerEnd.y}
    A ${insetInner} ${insetInner} 0 ${largeArc} 0 ${innerStart.x} ${innerStart.y}
    Z
  `;
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

  // Extract current values for stable dependency arrays
  const outerCurrent = outerConfig.current;
  const innerCurrent = innerConfig.current;

  // Cumulative rotation state - can exceed 360 degrees for smooth boundary crossing
  const [outerRotation, setOuterRotation] = useState(-outerCurrent * OUTER_STEP + OUTER_OFFSET);
  const [innerRotation, setInnerRotation] = useState(-innerCurrent * INNER_STEP + INNER_OFFSET);
  const prevOuterRef = useRef(outerCurrent);
  const prevInnerRef = useRef(innerCurrent);

  // Update outer rotation with shortest path
  useEffect(() => {
    const prev = prevOuterRef.current;
    if (prev !== outerCurrent) {
      let delta = outerCurrent - prev;
      // Calculate shortest path across the boundary
      if (delta > OUTER_SEGMENTS / 2) delta -= OUTER_SEGMENTS;
      if (delta < -OUTER_SEGMENTS / 2) delta += OUTER_SEGMENTS;
      setOuterRotation(rot => rot - delta * OUTER_STEP);
      prevOuterRef.current = outerCurrent;
    }
  }, [outerCurrent]);

  // Update inner rotation with shortest path
  useEffect(() => {
    const prev = prevInnerRef.current;
    if (prev !== innerCurrent) {
      let delta = innerCurrent - prev;
      // Calculate shortest path across the boundary
      if (delta > INNER_SEGMENTS / 2) delta -= INNER_SEGMENTS;
      if (delta < -INNER_SEGMENTS / 2) delta += INNER_SEGMENTS;
      setInnerRotation(rot => rot - delta * INNER_STEP);
      prevInnerRef.current = innerCurrent;
    }
  }, [innerCurrent]);

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

  const renderGroupBorders = (
    count: number,
    outerR: number,
    innerR: number,
    wheelId: 'inner' | 'outer'
  ) => {
    const groups = wheelId === 'outer' ? OUTER_GROUPS : INNER_GROUPS;

    return groups.map((group, i) => {
      const path = createGroupArcPath(group.start, group.end, count, outerR, innerR);

      return (
        <path
          key={`group-${i}`}
          d={path}
          fill="none"
          stroke={group.color}
          strokeWidth={GROUP_BORDER.width}
          className="pointer-events-none"
        />
      );
    });
  };

  return (
    <div className="flex flex-col items-center w-full h-full">
      <Search color={COLORS.indicator} className="w-16 h-8 flex-shrink-0" />
      <div className="flex flex-row items-center w-full">
        <svg
          viewBox="0 0 312 312"
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
            {renderGroupBorders(OUTER_SEGMENTS, OUTER_OUTER_RADIUS, OUTER_INNER_RADIUS, 'outer')}
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
            {renderGroupBorders(INNER_SEGMENTS, INNER_OUTER_RADIUS, INNER_INNER_RADIUS, 'inner')}
          </g>

          {/* Center circle */}
          <circle cx={CENTER} cy={CENTER} r="40" fill={COLORS.center} />

          {/* Static outer indicator dots (outside carousel, don't rotate) */}
          {OUTER_INDICATOR_DOTS.map((dot, i) => {
            const angleRad = ((dot.cuvette * (360 / OUTER_SEGMENTS)) - 90) * (Math.PI / 180);
            const x = CENTER + OUTER_INDICATOR_DOT_DISTANCE * Math.cos(angleRad);
            const y = CENTER + OUTER_INDICATOR_DOT_DISTANCE * Math.sin(angleRad);
            return (
              <g
                key={`outer-indicator-${i}`}
                className="cursor-pointer"
                onClick={() => outerConfig.onClick(dot.targetCuvette - 1)}
              >
                {/* Larger invisible hit area */}
                <circle cx={x} cy={y} r={dot.radius * 3} fill="transparent" />
                <circle
                  cx={x}
                  cy={y}
                  r={dot.radius}
                  fill={dot.color}
                  className="hover:opacity-80 transition-opacity"
                />
              </g>
            );
          })}

          {/* Static inner indicator dots (inside center circle, don't rotate) */}
          {INNER_INDICATOR_DOTS.map((dot, i) => {
            const angleRad = ((dot.cuvette * (360 / INNER_SEGMENTS)) - 90) * (Math.PI / 180);
            const x = CENTER + INNER_INDICATOR_DOT_DISTANCE * Math.cos(angleRad);
            const y = CENTER + INNER_INDICATOR_DOT_DISTANCE * Math.sin(angleRad);
            return (
              <g
                key={`inner-indicator-${i}`}
                className="cursor-pointer"
                onClick={() => innerConfig.onClick(dot.targetCuvette - 1)}
              >
                {/* Larger invisible hit area */}
                <circle cx={x} cy={y} r={dot.radius * 3} fill="transparent" />
                <circle
                  cx={x}
                  cy={y}
                  r={dot.radius}
                  fill={dot.color}
                  className="hover:opacity-80 transition-opacity"
                />
              </g>
            );
          })}
        </svg>
      </div>
      <ChevronUp color={COLORS.indicator} className="w-16 h-8 flex-shrink-0" />
    </div>
  );
};

export default CarouselDial;

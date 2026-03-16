import { FC, useState } from "react";
import { BaseCameraComponentProps } from "../CameraComponent.tsx";
import { Switch, Slider, Select, SelectItem } from "@nextui-org/react";
import { StreamingState } from "../hooks/useCameraStream.ts";
import { CameraComponent } from "../CameraComponent.tsx";
import Overlay from "../../../shared/components/Overlay/Overlay.tsx";
import CameraVideo, { CameraVideoProps } from "../components/CameraVideo.tsx";

/**
 * Full-width graduated ruler overlay for the microscope camera.
 * The ruler spans the full width or height of the video feed depending on position.
 * The user can adjust what real-world distance (in mm) that span represents.
 */

const MIN_SCALE_MM = 3;
const MAX_SCALE_MM = 60;
const DEFAULT_SCALE_MM = 20;

const TICK_SHADOW = "0 0 3px rgba(0,0,0,0.9)";
const LABEL_SHADOW = "0 0 4px rgba(0,0,0,0.9)";

type ScalePosition = "top" | "bottom" | "left" | "right";

/**
 * Compute a nice major tick interval for the given total mm range.
 */
function getMajorInterval(totalMm: number): number {
  if (totalMm <= 5) return 1;
  if (totalMm <= 15) return 2;
  if (totalMm <= 30) return 5;
  return 10;
}

const TICK_STEP = 0.5;

const HorizontalRuler: FC<{ totalMm: number; position: "top" | "bottom" }> = ({ totalMm, position }) => {
  const majorInterval = getMajorInterval(totalMm);
  const ticks = [];
  const tickCount = Math.round(totalMm / TICK_STEP);
  const isTop = position === "top";

  for (let i = 0; i <= tickCount; i++) {
    const mm = +(i * TICK_STEP).toFixed(1);
    const pct = (mm / totalMm) * 100;
    const isWhole = mm % 1 === 0;
    const isMajor = isWhole && mm % majorInterval === 0;
    const tickH = isMajor ? 16 : isWhole ? 10 : 6;

    ticks.push(
      <div key={mm} className="absolute" style={{ left: `${pct}%` }}>
        <div
          className="absolute bg-white"
          style={{
            width: isMajor ? 2 : 1,
            height: tickH,
            ...(isTop ? { top: 0 } : { bottom: 0 }),
            transform: "translateX(-50%)",
            boxShadow: TICK_SHADOW,
          }}
        />
        {isMajor && (
          <span
            className="absolute text-white font-semibold select-none"
            style={{
              fontSize: 10,
              ...(isTop ? { top: 18 } : { bottom: 18 }),
              transform: "translateX(-50%)",
              whiteSpace: "nowrap",
              textShadow: LABEL_SHADOW,
            }}
          >
            {mm}
          </span>
        )}
      </div>
    );
  }

  const containerClass = isTop
    ? "absolute top-4 left-4 right-4 pointer-events-none select-none"
    : "absolute bottom-10 left-4 right-4 pointer-events-none select-none";

  return (
    <div className={containerClass}>
      <div className="relative w-full" style={{ height: 40 }}>
        <div
          className="absolute w-full bg-white"
          style={{
            height: 2,
            ...(isTop ? { top: 0 } : { bottom: 0 }),
            boxShadow: TICK_SHADOW,
          }}
        />
        {ticks}
      </div>
      <span
        className="text-white text-xs font-bold mt-0.5 block select-none"
        style={{ textShadow: LABEL_SHADOW }}
      >
        mm
      </span>
    </div>
  );
};

const VerticalRuler: FC<{ totalMm: number; position: "left" | "right" }> = ({ totalMm, position }) => {
  const majorInterval = getMajorInterval(totalMm);
  const ticks = [];
  const tickCount = Math.round(totalMm / TICK_STEP);
  const isLeft = position === "left";

  for (let i = 0; i <= tickCount; i++) {
    const mm = +(i * TICK_STEP).toFixed(1);
    const pct = (mm / totalMm) * 100;
    const isWhole = mm % 1 === 0;
    const isMajor = isWhole && mm % majorInterval === 0;
    const tickW = isMajor ? 16 : isWhole ? 10 : 6;

    ticks.push(
      <div key={mm} className="absolute" style={{ top: `${pct}%` }}>
        <div
          className="absolute bg-white"
          style={{
            height: isMajor ? 2 : 1,
            width: tickW,
            ...(isLeft ? { left: 0 } : { right: 0 }),
            transform: "translateY(-50%)",
            boxShadow: TICK_SHADOW,
          }}
        />
        {isMajor && (
          <span
            className="absolute text-white font-semibold select-none"
            style={{
              fontSize: 10,
              ...(isLeft ? { left: 18 } : { right: 18 }),
              transform: "translateY(-50%)",
              whiteSpace: "nowrap",
              textShadow: LABEL_SHADOW,
            }}
          >
            {mm}
          </span>
        )}
      </div>
    );
  }

  const containerClass = isLeft
    ? "absolute top-4 bottom-10 left-4 pointer-events-none select-none"
    : "absolute top-4 bottom-10 right-4 pointer-events-none select-none";

  return (
    <div className={containerClass}>
      <div className="relative h-full" style={{ width: 40 }}>
        <div
          className="absolute h-full bg-white"
          style={{
            width: 2,
            ...(isLeft ? { left: 0 } : { right: 0 }),
            boxShadow: TICK_SHADOW,
          }}
        />
        {ticks}
      </div>
    </div>
  );
};

const ScaleRulerOverlay: FC<{ totalMm: number; position: ScalePosition }> = ({ totalMm, position }) => {
  if (position === "left" || position === "right") {
    return <VerticalRuler totalMm={totalMm} position={position} />;
  }
  return <HorizontalRuler totalMm={totalMm} position={position} />;
};

const POSITION_OPTIONS: { key: ScalePosition; label: string }[] = [
  { key: "bottom", label: "Bottom" },
  { key: "top", label: "Top" },
  { key: "left", label: "Left" },
  { key: "right", label: "Right" },
];

const MicroscopeScaleOverlayedCameraComponent: FC<BaseCameraComponentProps> = (props) => {
  const [isStreaming, setIsStreaming] = useState(false);
  const [overlayToggle, setOverlayToggle] = useState(true);
  const [scaleMm, setScaleMm] = useState(DEFAULT_SCALE_MM);
  const [scalePosition, setScalePosition] = useState<ScalePosition>("bottom");

  const settingsFormChildren = (
    <>
      <Switch
        size="sm"
        isSelected={overlayToggle}
        onChange={() => setOverlayToggle(!overlayToggle)}
      >
        Toggle Scale
      </Switch>
      {overlayToggle && (
        <>
          <Slider
            size="sm"
            label="Scale (mm)"
            minValue={MIN_SCALE_MM}
            maxValue={MAX_SCALE_MM}
            step={0.5}
            value={scaleMm}
            onChange={(v) => setScaleMm(v as number)}
          />
          <Select
            size="sm"
            label="Scale Position"
            selectedKeys={[scalePosition]}
            onSelectionChange={(keys) => {
              const selected = Array.from(keys)[0] as ScalePosition;
              if (selected) setScalePosition(selected);
            }}
          >
            {POSITION_OPTIONS.map((opt) => (
              <SelectItem key={opt.key}>{opt.label}</SelectItem>
            ))}
          </Select>
        </>
      )}
    </>
  );

  const overlay = overlayToggle && isStreaming
    ? <ScaleRulerOverlay totalMm={scaleMm} position={scalePosition} />
    : undefined;

  const cameraVideo = ({ videoRef, filters }: CameraVideoProps) => (
    <Overlay overlay={overlay}>
      <CameraVideo videoRef={videoRef} filters={filters} />
    </Overlay>
  );

  return (
    <CameraComponent
      {...props}
      onStreamingStateChange={(s) => setIsStreaming(s === StreamingState.STREAMING)}
      cameraVideoComponent={cameraVideo}
      settingsFormChildren={settingsFormChildren}
    />
  );
};

export default MicroscopeScaleOverlayedCameraComponent;

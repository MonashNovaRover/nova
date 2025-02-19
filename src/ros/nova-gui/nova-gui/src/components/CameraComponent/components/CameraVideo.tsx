import React, { LegacyRef } from "react";
import { CameraFilters } from "../CameraComponent";

interface CameraVideoProps {
    videoRef: LegacyRef<HTMLVideoElement> | undefined;
    filters: CameraFilters;
}

const enableScroll = () => {
    document.removeEventListener('wheel', preventDefault, false)
}

const disableScroll = () => {
    document.addEventListener('wheel', preventDefault, {
        passive: false,
    })
}

// Allow for panning with the mouse
const onMouseMove = useCallback((event: React.MouseEvent<HTMLCanvasElement>) => {
    if (event.buttons !== 1)
        return;

    const bounds = gl.canvasRef.current?.getBoundingClientRect() ?? {width: 1, height: 1};
    const maxResolutionComp = Math.max(bounds.width, bounds.height);

    setMousePos(([x, y]) => [
        x + 2 * Math.PI * event.movementX / maxResolutionComp,
        y + 2 * Math.PI * event.movementY / maxResolutionComp
    ]);
}, [window.canvasRef]);

const preventDefault = (e: Event) => {
    e = e || window.event
    if (e.preventDefault) {
        e.preventDefault()
    }
    e.returnValue = false
}

const CameraVideo: React.FC<CameraVideoProps> = ({ videoRef, filters }) => {
    const scaling = filters.flipCamera ? "scaleX(-1)" : "scaleX(1)";
    const rotation = `rotate(${filters.rotation}deg)`;
    const contrast = `contrast(${filters.contrast}%)`;
    const brightness = `brightness(${filters.brightness}%)`;
    const inversion = `invert(${filters.invertCamera ? 1 : 0})`;

    if () { // Camera is not scimbal camera
        return (
            <video
                style={{
                    transform: `${scaling} ${rotation}`,
                    filter: `${contrast} ${brightness} ${inversion}`,
                }}
                controls={false}
                autoPlay
                loop
                muted
                playsInline
                ref={videoRef}
                className="z-0 w-full h-full object-cover"
            />
        );
    } else {
        return (
            <video
                style={{
                    transform: `${scaling} ${rotation}`,
                    filter: `${contrast} ${brightness} ${inversion}`,
                }}
                onMouseMove={onMouseMove}
                onMouseEnter={disableScroll}
                onMouseLeave={enableScroll}
                controls={false}
                autoPlay
                loop
                muted
                playsInline
                ref={videoRef}
                className="z-0 w-full h-full object-cover"
            />
        );
    }
};

export default CameraVideo;

//
// import React, { LegacyRef } from "react";
// import { CameraFilters } from "../CameraComponent";
//
// interface CameraVideoProps {
//   videoRef: LegacyRef<HTMLVideoElement> | undefined;
//   filters: CameraFilters;
// }
//
// const CameraVideo: React.FC<CameraVideoProps> = ({ videoRef, filters }) => {
//   const scaling = filters.flipCamera ? "scaleX(-1)" : "scaleX(1)";
//   const rotation = `rotate(${filters.rotation}deg)`;
//   const contrast = `contrast(${filters.contrast}%)`;
//   const brightness = `brightness(${filters.brightness}%)`;
//   const inversion = `invert(${filters.invertCamera ? 1 : 0})`;
//
//   return (
//     <video
//       style={{
//         transform: `${scaling} ${rotation}`,
//         filter: `${contrast} ${brightness} ${inversion}`,
//       }}
//       controls={false}
//       autoPlay
//       loop
//       muted
//       playsInline
//       ref={videoRef}
//       className="z-0 w-full h-full object-cover"
//     />
//   );
// };
//
// export default CameraVideo;

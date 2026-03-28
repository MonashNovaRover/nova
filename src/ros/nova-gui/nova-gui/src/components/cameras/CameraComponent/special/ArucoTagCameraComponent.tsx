import { BaseCameraComponentProps, CameraComponent } from "../CameraComponent.tsx";
import { FC } from "react";
import CameraVideo, { CameraVideoProps } from "../components/CameraVideo.tsx";
import { useArucoTagDetection } from "../hooks/useArucoTagDetection.ts";

const ArucoTagCameraComponent: FC<BaseCameraComponentProps> = (props) => {
    const cameraVideo = ({ videoRef, filters }: CameraVideoProps) => {
        useArucoTagDetection(videoRef as React.RefObject<HTMLVideoElement | null>);
        return <CameraVideo videoRef={videoRef} filters={filters} />;
    };
    return (
        <CameraComponent
            {...props}
            cameraVideoComponent={cameraVideo}
        />
    );
};

export default ArucoTagCameraComponent;
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import toast from "react-hot-toast";
import {useCallback, useMemo} from "react";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";

/**
 * Gets the camera serials that are online in the camera stack
 */
export const useOnlineCameraSerials = () => {
  const onlineCameras = useSelector(
    (state: RootState) => state.camerasStore.cameras
  );

  return useMemo(() => onlineCameras.map((cam) => cam.serial), [onlineCameras])
}

/**
 * Wraps the bifrost calls in a reusable hook.
 * Specify whether you want the toast popup to print "all" cam or individual serials
 * @param refreshAvailabilies
 */
export const useStreamingBifrost = (refreshAvailabilies: () => void) => {
  const bifrostStarter = useBifrost({ service: RosService.START_CAMS });
  const bifrostPauser = useBifrost({ service: RosService.PAUSE_CAMS });
  const bifrostStopper = useBifrost({ service: RosService.STOP_CAMS });

  const startStreaming = useCallback((serials: string[], useAllMessage: boolean) => {
    if (serials.length < 1) {
      toast.error("Cannot start streaming 0 serials")
      return
    }

    bifrostStarter.callService(
      {serials: serials},
      {
        responseToast: true,
        successToastMessage: useAllMessage ? "All Cameras Started Up!" : `${serials.join(", ")} Started!`,
        errorToastMessage: useAllMessage ? "Failed to start up all cameras." : `${serials.join(", ")} Failed to Start!`,
        handleResponse: () => refreshAvailabilies(),
      }
    )
  }, [bifrostStarter, refreshAvailabilies])

  const pauseStreaming = useCallback((serials: string[], useAllMessage: boolean) => {
    if (serials.length < 1) {
      toast.error("Cannot pause streaming 0 serials")
      return
    }

    bifrostPauser.callService(
      {serials: serials},
      {
        responseToast: true,
        successToastMessage: useAllMessage ? "All Cameras Paused!" : `${serials.join(", ")} Paused!`,
        errorToastMessage: useAllMessage ? "Failed to pause all cameras." : `${serials.join(", ")} Failed to Pause!`,
        handleResponse: () => refreshAvailabilies(),
      }
    )
  }, [bifrostPauser, refreshAvailabilies])

  const stopStreaming = useCallback((serials: string[], useAllMessage: boolean) => {
    if (serials.length < 1) {
      toast.error("Cannot stop streaming 0 serials")
      return
    }

    bifrostStopper.callService(
      {serials: serials},
      {
        responseToast: true,
        successToastMessage: useAllMessage ? "All Cameras Stopped!" : `${serials.join(", ")} Stopped!`,
        errorToastMessage: useAllMessage ? "Failed to stop all cameras." : `${serials.join(", ")} Failed to Stop!`,
        handleResponse: () => refreshAvailabilies(),
      }
    )
  }, [bifrostStopper, refreshAvailabilies])

  return [startStreaming, pauseStreaming, stopStreaming]
}

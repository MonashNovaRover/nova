import { useRef, useEffect, RefObject } from "react";
import toast from "react-hot-toast";
import BaileyWaving from "../../../../assets/meme/bailey.gif";
import Terry from "../../../../assets/meme/terry.gif";
import Meatman from "../../../../assets/meme/meatman.jpg";
import AR from "js-aruco2";

export function useArucoTagDetection(videoRef: RefObject<HTMLVideoElement | null>) {
  // https://stackoverflow.com/questions/60618844/useeffect-is-called-twice-even-if-an-empty-array-is-used-as-an-argument
  const intervalInitialised = useRef(false);

  useEffect(() => {
    if (intervalInitialised.current) {
      return;
    }
    intervalInitialised.current = true;

    AR.AR.DICTIONARIES.ARCh = {
      nBits: 16,
      tau: 2,
      codeList: [[181,50],[15,154],[51,45],[153,70],[84,158],[121,205]]
    };

    const detector = new AR.AR.Detector({
      dictionaryName: 'ARCh'
    });


    const interval = setInterval(async () => {
      if (videoRef.current && window) { const video = videoRef.current;
        const canvas = document.createElement("canvas");
        canvas.width = video.videoWidth;
        canvas.height = video.videoHeight;
        const context = canvas.getContext("2d");
        if (context && canvas.width && canvas.height) {
          context.drawImage(video, 0, 0);

          const imageData = context.getImageData(0,0, canvas.width, canvas.height);
          const markers = detector.detect(imageData);
          const toastProps = {
            position: 'bottom-right',
            duration: 2000,
            style: {
              //maxHeight: "100px"
            }
          };
          const ids = markers.map((x)=> {
            switch (x.id) {
              case 0:
                toast(
                  <span>
                    TAG 0!!
                    <img src={BaileyWaving} width="100" height="200" />
                  </span>,
                  {...toastProps, id: "tag0"}
                );
                break;
              case 1:
                toast(
                  <span>
                    TAG 1!!
                    <img src={Meatman} width="100" height="200" />
                  </span>,
                  {...toastProps, id: "tag1"}
                );
                break;
              case 2:
                toast("tag 2 safety conscious stegosaurus", {...toastProps, id: "tag2"});
                break;
              case 3:
                toast(
                  <span>
                    TAG 3!!
                    <img src={Terry} width="100" height="200" />
                  </span>,
                  {...toastProps, id: "tag3"}
                );
                break;
              case 4:
                toast("tag 4 boat & moon", {...toastProps, id: "tag4"});
                break;
              case 5:
                toast("tag 5 ???", {...toastProps, id: "tag5"});
                break;
            }
          });
        }
      }
      return () => clearInterval(interval);
    }, 1000);
  }, [videoRef]);
}

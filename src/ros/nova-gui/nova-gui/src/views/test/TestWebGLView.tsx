
import MicroscopeThresholdWidget from "../../components/MicroscopeThresholdWidget/MicroscopeThresholdWidget.tsx";

const TestWebGLView: React.FC = () => {
  return (<>
    <div className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-1 md:grid-cols-1 lg:grid-cols-2 2xl:grid-cols-3">
      <MicroscopeThresholdWidget cameraSerial={"science_microscope"}/>
    </div>
  </>)
};

export default TestWebGLView;

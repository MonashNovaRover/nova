import NIRProbeLEDWidget from "../../components/nir-probe/NIRProbeLEDWidget/NIRProbeLEDWidget.tsx";
import NIRProbeValueWidget from "../../components/nir-probe/NIRProbeOutputWidget/NIRProbeValueWidget.tsx";

const TestScienceView: React.FC = () => {
  return (
    <div className="grid  w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-6">
      <NIRProbeLEDWidget/>
      <NIRProbeValueWidget/>
    </div>
  )
};

export default TestScienceView;

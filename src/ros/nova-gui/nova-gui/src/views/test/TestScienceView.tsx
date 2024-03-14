import NIRProbeWidget from "../../components/nir-probe/NIRProbeOutputWidget/NIRProbeWidget.tsx";

const TestScienceView: React.FC = () => {
  return (
    <div className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-1 md:grid-cols-1 lg:grid-cols-2 2xl:grid-cols-3">
      <NIRProbeWidget/>

    </div>
  )
};

export default TestScienceView;

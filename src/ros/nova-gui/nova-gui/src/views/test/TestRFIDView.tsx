import RFIDWidget from "../../components/RFIDWidget/RFIDWidget.tsx";

const TestRFIDView: React.FC = () => {
  return (
    <>
      <div className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-6">
        <RFIDWidget className="col-span-2"></RFIDWidget>
      </div>
    </>
  );
};

export default TestRFIDView;

import React from "react";
import DriveJoyWidget from "../../../components/drive/DriveJoyWidget/DriveJoyWidget.tsx";
import ArmJoyWidget from "../../../components/arm/ArmJoyWidget/ArmJoyWidget.tsx";


const TestJoyView: React.FC = () => {
  return <div className="max-h-full">
    <DriveJoyWidget />
    <ArmJoyWidget className="mt-2"/>
</div>
};

export default TestJoyView;

import React from "react";
import URDFWidget from "../../../components/navbar/URDFWidget/URDFWidget.tsx";


const TestUrdfView: React.FC = () => {
  return <div className="max-h-full">
    <URDFWidget 
      urdfPath="/rover_description/banksia/urdf/rover.urdf" 
      packages={{ robot: "/models/your_pkg_name/" }}
    />
    
</div>
};

export default TestUrdfView;

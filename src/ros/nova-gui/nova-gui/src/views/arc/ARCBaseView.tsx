import React from "react";
import novaLogo from "../../assets/nova-logo.png";

const ARCBaseView: React.FC = () => {
  return (
    <div className="flex justify-center items-center h-full w-full">
      <img src={novaLogo} className="w-5/12 opacity-60" alt="Nova Logo" />
    </div>
  );
};

export default ARCBaseView;

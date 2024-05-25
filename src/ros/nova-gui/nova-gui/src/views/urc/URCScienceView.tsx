import React from "react";
import PlatformWidget from "../../components/PlatformWidget/PlatformWidget";
import HydroprobeWidget from "../../components/HydroprobeWidget/HydroprobeWidget";


const URCScienceView: React.FC = () => {
  return (
    <div className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-2 lg:grid-cols-4 2xl:grid-cols-6">
      <HydroprobeWidget className="row-start-1 w-full col-span-3" />
      <PlatformWidget />
    </div>
  );
};

export default URCScienceView;

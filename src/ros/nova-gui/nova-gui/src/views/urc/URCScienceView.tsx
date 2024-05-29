import React from "react";
import PlatformWidget from "../../components/PlatformWidget/PlatformWidget";
import HydroprobeWidget from "../../components/HydroprobeWidget/HydroprobeWidget";
import CarouselWidget from "../../components/CarouselWidget/CarouselWidget";
import TOFHeight from "../../components/AnalysisPlatformHeight/AnalysisPlatformHeight";
import PumpsWidget from "../../components/PumpsWidget/PumpsWidget";


const URCScienceView: React.FC = () => {
  return (
    <div className="grid w-full gap-3 p-3 grid-cols-6">
      <HydroprobeWidget className="row-start-1 w-full col-span-2" />
      <TOFHeight className="row-start-1 w-full col-span-1"/>
      <PlatformWidget className="row-start-2 w-full col-span-2" />
      <CarouselWidget className="row-start-2 w-full col-span-2"/>
      <PumpsWidget className="row-start-3 w-full col-span-2"/>
     
    </div>
  );
};

export default URCScienceView;

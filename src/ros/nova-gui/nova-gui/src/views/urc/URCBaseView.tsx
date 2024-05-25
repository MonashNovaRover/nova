import RamanSpec from "../../components/RamanSpec/RamanSpec";

import UVVisSpec from "../../components/UVVisSpec/UVVisSpec.tsx";

const URCBaseView: React.FC = () => {
  return <div className="m-3 grid grid-cols-2 gap-3">
    <UVVisSpec />
    <RamanSpec />
  </div>;
};

export default URCBaseView;

import WebGL360CamWidget from "../../components/WebGL360CamWidget/WebGL360CamWidget.tsx";

const TestWebGLView: React.FC = () => {
  return (<>
    <div className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-6">
      <WebGL360CamWidget className="row-start-1 w-full col-span-2"/>
    </div>
  </>)
};

export default TestWebGLView;

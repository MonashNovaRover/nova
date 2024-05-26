import Theta360CamWidget from "../../../components/Theta360CamWidget/Theta360CamWidget.tsx";


export default function Test360CamView() {

  return (<div className="h-screen">
    <div className="grid w-full gap-3 p-3 auto-cols-fr max-h-full grid-cols-1 overflow-clip pb-48">
      <Theta360CamWidget></Theta360CamWidget>
    </div>
  </div>)
}
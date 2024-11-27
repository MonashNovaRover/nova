
import { expect, test } from 'vitest'
import { render } from 'vitest-browser-react'
//import { App } from '../main.tsx'
import App from './NovaGui.tsx'

test('TopBar Empty', async () => {
  const gui = render(<App/>)
  console.log(gui)
  await 0;
  //await expect.element(getByText('Hello Vitest!')).toBeInTheDocument()
})


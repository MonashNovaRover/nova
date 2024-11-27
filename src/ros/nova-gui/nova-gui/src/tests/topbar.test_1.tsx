/*
import { expect, test } from 'vitest'
import { render } from 'vitest-browser-react'
import TopBar from '../components/TopBar/TopBar.tsx'

test('TopBar Empty', async () => {
  const gui = render(<TopBar/>)
  console.log(gui)
  await 0;
  //await expect.element(getByText('Hello Vitest!')).toBeInTheDocument()
})
*/
import { expect, test } from 'vitest'
import { render, screen } from 'vitest-browser-react'
import { NovaTopBar } from '../components/TopBar/TopBar.tsx'
import { Provider } from 'react-redux/src'

const ProviderWrapper = ({ children }) => (
  <Provider store={store}>
    <BrowserRouter>
      {children}
    </BrowserRouter>
  </Provider>
);

test('TopBar Empty', async () => {
  const gui = render(<NovaTopBar/>, { wrapper: ProviderWrapper })
  console.log(gui)
  await 0;
  //await expect.element(getByText('Hello Vitest!')).toBeInTheDocument()
})

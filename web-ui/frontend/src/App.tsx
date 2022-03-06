import { Input } from 'reactstrap';
import { MdApps, MdDarkMode, MdLightMode } from 'react-icons/md';
import { useEffect, useState } from 'react';
import { Application, AppSession, Session } from './interfaces';

const MS_MINUTE = 60000;

async function getData(
  start: Date,
  end: Date
): Promise<AppSession | undefined> {
  await fetch(
    process.env.REACT_APP_API_BASE_URL ??
      'http://localhost:29687' +
        '/data?' +
        new URLSearchParams({
          start: start.toISOString(),
          end: end.toISOString(),
        })
  ).then((res: Response) => {
    console.log(res.body);
  });

  return Promise.resolve(undefined);
}

function App() {
  const [render, rerender] = useState(false);
  const [start, setStart] = useState(
    new Date(new Date().getTime() - 5 * MS_MINUTE)
  );
  const [end, setEnd] = useState(new Date());
  const [isToPresent, setToPresent] = useState(true);

  useEffect(() => {
    console.log(start, end);

    getData(start, end);
    const getDataIntervalId = setInterval(async () => {
      await getData(start, end);
    }, 3000);

    return () => clearInterval(getDataIntervalId);
  }, [start, end, isToPresent]);

  const isDarkMode =
    localStorage.getItem('theme') === 'dark' ||
    (!('theme' in localStorage) &&
      window.matchMedia('(prefers-color-scheme: dark)').matches);

  if (isDarkMode) {
    document.documentElement.classList.add('dark');
  } else {
    document.documentElement.classList.remove('dark');
  }

  return (
    <div className="h-screen p-4 bg-gray-100 dark:bg-black">
      <div className="h-full rounded-lg bg-white dark:bg-gray-800">
        <div className="w-full pt-4 px-4 flex">
          <Input
            className="input-datetime"
            type="datetime-local"
            name="dateFrom"
            id="dateFrom"
            placeholder="datetime placeholder"
            value={start.toISOString().substring(0, 16)}
            onChange={(e) => {
              setStart(new Date(e.target.value));
            }}
            max={end.toISOString().substring(0, 16)}
          />
          <span className="flex items-center mx-4 text-xl font-semibold dark:text-white">
            -
          </span>
          <Input
            className="input-datetime"
            type="datetime-local"
            name="dateFrom"
            id="dateFrom"
            placeholder="datetime placeholder"
            value={end.toISOString().substring(0, 16)}
            onChange={(e) => setEnd(new Date(e.target.value))}
            min={start.toISOString().substring(0, 16)}
            disabled={isToPresent}
          />
          <button
            className={
              'text-sm min-w-max ml-4 px-4 rounded-lg font-semibold transition-all shadow-md ' +
              'dark:text-white ' +
              (isToPresent
                ? 'bg-blue-600 text-white dark:bg-gray-100 dark:text-blue-600'
                : 'text-blue-600 hover:bg-gray-100 hover:text-blue-600 dark:hover:text-blue-600')
            }
            onClick={() => {
              setToPresent(!isToPresent);
              setEnd(new Date());
            }}
          >
            TO PRESENT
          </button>
          <div className="w-full flex flex-row-reverse pl-4">
            <button
              className="ml-4 p-2"
              onClick={() => {
                isDarkMode
                  ? localStorage.setItem('theme', 'light')
                  : localStorage.setItem('theme', 'dark');
                rerender(!render);
              }}
            >
              {isDarkMode ? (
                <MdLightMode size={25} className="text-white" />
              ) : (
                <MdDarkMode size={25} />
              )}
            </button>

            <button className="button-primary">
              <MdApps className="mr-1" size={20} /> 20 APPS
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;

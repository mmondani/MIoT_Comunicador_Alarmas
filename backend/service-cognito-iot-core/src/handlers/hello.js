async function hello(event, context) {
  return {
    statusCode: 200,
    body: JSON.stringify({ }),
  };
}

export const handler = hello;



import HTML from '../index.html';

export default {
  async fetch(request, env) {
    const auth = request.headers.get('Authorization');
    if (!auth?.startsWith('Basic ')) return unauthorized();
    const [user, pass] = atob(auth.slice(6)).split(':');
    if (user !== env.AUTH_USER || pass !== env.AUTH_PASS) return unauthorized();
    return new Response(HTML, { headers: { 'Content-Type': 'text/html' } });
  }
};

function unauthorized() {
  return new Response('Unauthorized', {
    status: 401,
    headers: { 'WWW-Authenticate': 'Basic realm="Irrigation Controller"' }
  });
}

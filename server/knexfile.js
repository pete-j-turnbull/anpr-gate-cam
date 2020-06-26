const pg = require('pg');

if (process.env.NODE_ENV !== 'local') pg.defaults.ssl = true;

const cConf = Object.assign(
  { host: process.env.PG_HOST, port: process.env.PG_PORT, database: process.env.PG_DB },
  process.env.NODE_ENV !== 'local'
    ? { user: process.env.PG_USER, password: process.env.PG_PASS, ssl: { rejectUnauthorized: false } }
    : {},
);

module.exports = {
  client: 'postgresql',
  connection: cConf,
  pool: { min: 1, max: 1 },
  migrations: { tableName: 'knex_migrations' },
};

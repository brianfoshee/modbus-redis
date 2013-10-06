# Back up the RPi's data to my laptop to conserve memory
# until a proper script can be written to do it
# on an hourly basis from the Pi itself.
# ssh -L 6378:localhost:6379 user@RPi_ip

require 'redis'

$pi_r = Redis.new(host: '127.0.0.1', port: 6378, timeout: 60)
$my_r = Redis.new(host: '127.0.0.1', port: 6379)

def migrate2
  pi_keys = $pi_r.keys '*'
  pi_keys.delete 'timestamps'

  pi_keys.each do |key|
    keyvals = $pi_r.hgetall key
    keyvals.each do |k,v|
      $my_r.sadd "timestamps", k
      $my_r.hset key, k, v
      $pi_r.hdel key, k
    end
  end
end

# Migrate first way of storing data to new layout on my laptop
def migrate1
  pi_keys = $pi_r.keys "*:*"

  pi_keys.each do |key|
    m = key.match /(?<dat>\w+)\:(?<ts>\d+)/
    tstamp = m[:ts]
    dat = m[:dat]
    val = $pi_r.get key
    $my_r.hset dat, tstamp, val
    $pi_r.del key
    puts "Setting #{dat} -> #{tstamp} to #{val}"
  end
end

migrate2

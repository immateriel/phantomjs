#!/usr/local/rvm/rubies/ruby-1.9.3-p392/bin/ruby
require 'pp'

class PhantomJsPool
  @pid_basename="/var/run/phantomjs"
  @log_basename="/var/log/phantomjs"

#  @pid_basename="./run/phantomjs"
#  @log_basename="/tmp/phantomjs"

  @daemon="/home/julbouln/phantomjs/bin/phantomjs"
#  @daemon="/usr/bin/tail -f /dev/null"
  @host="127.0.0.1"
  @port=12000
  @instances=5

  def self.pid_filename(i)
    "#{@pid_basename}.#{@port+i}.pid"
  end

  def self.log_filename(i)
    "#{@log_basename}.#{@port+i}.log"
  end

  def self.err_filename(i)
    "#{@log_basename}.#{@port+i}.err"
  end

  def self.start
    @instances.times do |i|
      self.start_instance(i)
    end
  end

  def self.start_instance(i)
    puts "start phantomjs on #{@host}:#{@port+i}"

    pid=fork do
      exec("exec #{@daemon} --host #{@host} --port #{@port+i} > #{self.log_filename(i)} 2> #{self.err_filename(i)}")
    end

    File.open(self.pid_filename(i), 'w') { |file| file.write(pid.to_s) }
  end

  def self.stop
    @instances.times do |i|
      puts "stop phantomjs on #{@host}:#{@port+i}"
      if File.exists?(self.pid_filename(i))
        pid=File.read(self.pid_filename(i))
        cmd="kill #{pid}"
        puts cmd
        system(cmd)
        File.delete(self.pid_filename(i))
      else
        puts "phantomjs pid not found for #{@host}:#{@port+i}"
      end
    end
  end

  def self.force_stop
    @instances.times do |i|
      pid=self.find_pid(i)
      if pid
        system("kill #{pid}")
      end
    end
    @instances.times do |i|
      pid=self.find_pid(i)
      if pid
        system("kill -11 #{pid}")
      end
    end
    @instances.times do |i|
      pid=self.find_pid(i)
      if pid
        system("kill -9 #{pid}")
      end
    end
    system("killall phantomjs")
    system("killall -11 phantomjs")
    system("killall -9 phantomjs")
  end

  def self.find_pid(i)
    res=`ps ax|grep phantomjs|grep -v grep|grep "port #{@port+i}"`.split(/\n/).first
    if res
      pid=res.strip.split(/\s/).first
      if pid!=""
        pid
      end
    end
  end

  def self.running?(i)
    if File.exists?(self.pid_filename(i))
      pid=File.read(self.pid_filename(i))
      if system("kill -0 #{pid}")
        true
      else
        File.delete(self.pid_filename(i))
        false
      end
    else
      false
    end
  end

  def self.restart_if_dead
    @instances.times do |i|
      unless self.running?(i)
        self.start_instance(i)
      end
    end
  end

  def self.parse(arg)
    if arg
      case arg
        when "start"
          self.start
        when "stop"
          self.stop
        when "force_stop"
          self.force_stop
        when "restart_with_force"
          self.force_stop
          sleep 1
          self.start
        when "restart"
          self.stop
          self.start
        when "restart_if_dead"
          self.restart_if_dead
        when "test"
          puts self.running?(1)
          puts self.find_pid(1)
      end
    end
  end
end

PhantomJsPool.parse(ARGV[0])

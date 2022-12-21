require 'socket'
starttime = Process.clock_gettime(Process::CLOCK_MONOTONIC)

s = TCPSocket.new 'localhost', 8989

s.write("/Users/adele/Desktop/College/NetworkAssignment1/#{ARGV[0]}.txt\n")

s.each_line do |line|
    puts line
end

s.close
endtime = Process.clock_gettime(Process::CLOCK_MONOTONIC)
elapsed = endtime - starttime
puts "Elapsed: #{elapsed} (#{ARGV[0]})"
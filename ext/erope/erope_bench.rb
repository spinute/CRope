load './erope.rb'

TRIAL = 100
#GC.disable

1.step(20, 1) do |n|
	puts "n=#{n}"
	t = Time.now
	TRIAL.times do
		e = String.new "a"*5
		n.times do
			e += e
		end
	end
	puts "String time: #{Time.now - t}"

	t = Time.now
	TRIAL.times do
		e = Rope.new "a"*5
		n.times do
			e += e
		end
	end
	puts "Rope time: #{Time.now - t}"

	t = Time.now
	TRIAL.times do
		e = ElasticRope.new "a"*5
		n.times do
			e += e
		end
	end
	puts "ERope time: #{Time.now - t}"
end

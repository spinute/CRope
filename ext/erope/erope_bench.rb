load './erope.rb'
require 'gnuplot'

def gen_plot(plot, yy, title, step= 1)
	xx = (1..15).map{|i|i * step}

	yy.map!{|e|e.to_f}

	plot.data << Gnuplot::DataSet.new([xx, yy]) do |ds|
		ds.with = "linespoints"
		ds.title = title
	end
end

TRIAL = 1000
len = 100
res1 = []
res2 = []
res3 = []
res4 = []

1.step(15, 1) do |n|
	puts "n=#{n}"
	t = Time.now
	TRIAL.times do
		e = String.new "a"*len
		n.times do
			e << e
		end
	end
	res = Time.new - t
	puts "String(concat) time: #{res}"
	res1 << res

	t = Time.now
	TRIAL.times do
		e = String.new "a"*len
		n.times do
			e = e + e
		end
	end
	res = Time.new - t
	puts "String(plus) time  : #{res}"
	res2 << res

	t = Time.now
	TRIAL.times do
		e = Rope.new "a"*len
		n.times do
			e += e
		end
	end
	res = Time.new - t
	puts "Rope time        : #{res}"
	res3 << res

	t = Time.now
	TRIAL.times do
		e = ElasticRope.new "a"*len
		n.times do
			e += e
		end
	end
	res = Time.new - t
	puts "ERope time       : #{res}"
	res4 << res
end

Gnuplot.open do |gp|
	Gnuplot::Plot.new(gp) do |plot|
		plot.xrange "[0:20]"
		plot.ylabel "time(s)"
		plot.xlabel "n"
		plot.title "double"
		gen_plot plot, res1, "String(concat)"
		gen_plot plot, res2, "String(plus)"
		gen_plot plot, res3, "Rope"
		gen_plot plot, res4, "ERope"
	end
end

res1 = []
res2 = []
res3 = []
res4 = []

50.step(500, 50) do |n|
	puts "n=#{n}"
	t = Time.now
	TRIAL.times do
		e = String.new "a"*len
		s = ""
		n.times do
			s << e
		end
	end
	res = Time.new - t
	puts "String(con) time: #{res}"
	res1 << res

	t = Time.now
	TRIAL.times do
		e = String.new "a"*len
		s = ""
		n.times do
			s +=e
		end
	end
	res = Time.new - t
	puts "String time      : #{res}"
	res2 << res

	t = Time.now
	TRIAL.times do
		e = Rope.new "a"*len
		s = Rope.new ""
		n.times do
			s += e
		end
	end
	res = Time.new - t
	puts "Rope time        : #{res}"
	res3 << res

	t = Time.now
	TRIAL.times do
		e = ElasticRope.new "a"*len
		s = ElasticRope.new ""
		n.times do
			s += e
		end
	end
	res = Time.new - t
	puts "ERope time       : #{res}"
	res4 << res
end

Gnuplot.open do |gp|
	Gnuplot::Plot.new(gp) do |plot|
		plot.xrange "[0:500]"
		plot.ylabel "time(s)"
		plot.xlabel "n"
		plot.title "append"
		gen_plot plot, res1, "String(concat)", 50
		gen_plot plot, res2, "String(plus)", 50
		gen_plot plot, res3, "Rope", 50
		gen_plot plot, res4, "ERope", 50
	end
end

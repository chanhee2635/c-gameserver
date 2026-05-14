using Microsoft.EntityFrameworkCore;
using LoginWebServer.Models;
using StackExchange.Redis;
using LoginWebServer.Services;

var builder = WebApplication.CreateBuilder(args);

// MySQL
var connectionString = builder.Configuration.GetConnectionString("MySQL")!;

builder.Services.AddDbContextPool<AppDbContext>(options =>
    options.UseMySql(connectionString, ServerVersion.AutoDetect(connectionString)));

// Redis
var redisConn = builder.Configuration["Redis:ConnectionString"]!; 
builder.Services.AddSingleton<IConnectionMultiplexer>(
    ConnectionMultiplexer.Connect(redisConn));

builder.Services.AddScoped<AuthService>();

// Controllers + Swagger
builder.Services.AddControllers().AddJsonOptions(options =>
{
    options.JsonSerializerOptions.PropertyNamingPolicy = null;
});
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

var app = builder.Build();

if (app.Environment.IsDevelopment())
{
    app.UseSwagger();
    app.UseSwaggerUI();
}

app.UseAuthorization();
app.MapControllers();
app.Run();
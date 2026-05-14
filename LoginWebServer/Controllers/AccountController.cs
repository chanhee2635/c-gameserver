using LoginWebServer.Services;
using Microsoft.AspNetCore.Mvc;

namespace LoginWebServer.Controllers;

[ApiController]
[Route("api/auth")]
public class AccountController : ControllerBase
{
    private readonly AuthService _authService;

    public AccountController(AuthService authService)
    {
        _authService = authService;
    }

    [HttpPost("create")]
    public Task<CreateAccountRes> CreateAccount([FromBody] CreateAccountReq req)
        => _authService.CreateAccountAsync(req);

    [HttpPost("login")]
    public Task<LoginAccountRes> LoginAccount([FromBody] LoginAccountReq req)
        => _authService.LoginAccountAsync(req);
}